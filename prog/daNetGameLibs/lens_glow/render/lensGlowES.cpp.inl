// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/componentTypes.h>
#include <render/renderSettings.h>
#include <ecs/render/updateStageRender.h>
#include <render/renderEvent.h>
#include <render/resolution.h>
#include <render/renderer.h>
#include <render/lensGlow/render/lensGlowCommon.h>

template <typename Callable>
static void lens_glow_enabled_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);

static bool query_lens_glow_enabled(ecs::EntityManager &manager)
{
  bool enabled = false;
  ecs::EntityId settingEid = manager.getSingletonEntity(ECS_HASH("render_settings"));
  lens_glow_enabled_ecs_query(manager, settingEid, [&enabled](bool render_settings__lensFlares, bool render_settings__bare_minimum) {
    enabled = render_settings__lensFlares && !render_settings__bare_minimum;
  });
  return enabled;
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear, SetResolutionEvent)
ECS_TRACK(enabled,
  lens_glow_config__bloom_offset,
  lens_glow_config__bloom_multiplier,
  lens_glow_config__lens_flare_offset,
  lens_glow_config__lens_flare_multiplier,
  lens_glow_config__texture_variants,
  lens_glow_config__texture_intensity_multiplier,
  lens_glow_config__texture_tint)
static void lens_glow_apply_settings_es(const ecs::Event &,
  ecs::EntityManager &manager,
  bool enabled,
  float lens_glow_config__bloom_offset,
  float lens_glow_config__bloom_multiplier,
  float lens_glow_config__lens_flare_offset,
  float lens_glow_config__lens_flare_multiplier,
  float lens_glow_config__texture_intensity_multiplier,
  const Point3 &lens_glow_config__texture_tint,
  const ecs::Array &lens_glow_config__texture_variants)
{
  if (get_world_renderer() == nullptr)
    return; // In this case, it will be called again anyway
  IPoint2 display_resolution = get_display_resolution();
  const auto texture = pick_best_texture(lens_glow_config__texture_variants, display_resolution);
  apply_lens_glow_settings(enabled && query_lens_glow_enabled(manager), display_resolution, lens_glow_config__bloom_offset,
    lens_glow_config__bloom_multiplier, lens_glow_config__lens_flare_offset, lens_glow_config__lens_flare_multiplier, texture,
    lens_glow_config__texture_intensity_multiplier, lens_glow_config__texture_tint);
}

template <typename Callable>
static void lens_glow_ecs_query(ecs::EntityManager &manager, Callable c);

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__lensFlares, render_settings__bare_minimum)
// Option is shared with lens flares on purpose
static void lens_glow_on_settings_changed_es(
  const ecs::Event &, ecs::EntityManager &manager, bool render_settings__lensFlares, bool render_settings__bare_minimum)
{
  bool lensFlaresEnabled = render_settings__lensFlares && !render_settings__bare_minimum;
  lens_glow_ecs_query(manager, [lensFlaresEnabled](bool enabled, float lens_glow_config__bloom_offset,
                                 float lens_glow_config__bloom_multiplier, float lens_glow_config__lens_flare_offset,
                                 float lens_glow_config__lens_flare_multiplier, const ecs::Array &lens_glow_config__texture_variants,
                                 float lens_glow_config__texture_intensity_multiplier, const Point3 &lens_glow_config__texture_tint) {
    IPoint2 display_resolution = get_display_resolution();
    const auto texture = pick_best_texture(lens_glow_config__texture_variants, display_resolution);
    apply_lens_glow_settings(enabled && lensFlaresEnabled, display_resolution, lens_glow_config__bloom_offset,
      lens_glow_config__bloom_multiplier, lens_glow_config__lens_flare_offset, lens_glow_config__lens_flare_multiplier, texture,
      lens_glow_config__texture_intensity_multiplier, lens_glow_config__texture_tint);
  });
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(const ecs::Array &lens_glow_config__texture_variants)
static void lens_glow_disable_es(const ecs::Event &)
{
  apply_lens_glow_settings(false, IPoint2::ZERO, 0, 0, 0, 0, "", 0, Point3(0, 0, 0));
}