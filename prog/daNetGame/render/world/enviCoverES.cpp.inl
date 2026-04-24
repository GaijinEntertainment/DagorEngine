// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>

#include <render/renderEvent.h>
#include <render/enviCover/enviCoverVars.h>

#define INSIDE_RENDERER
#include <render/world/private_worldRenderer.h>

static void updateEnviCover(bool envi_cover)
{
  envi_cover_vars::set_envi_cover(envi_cover);
  WorldRenderer *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL("Tried to access WorldRenderer components when WorldRenderer did not exist!"));
  wr->setEnviCover(envi_cover);
}

ECS_TAG(render)
ECS_ON_EVENT(OnLevelLoaded)
static void envi_cover_es(const ecs::Event &, bool envi_cover) { updateEnviCover(envi_cover); }

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(bool envi_cover)
static void envi_cover_unload_es(const ecs::Event &) { updateEnviCover(false); }

ECS_TAG(render)
ECS_ON_EVENT(OnLevelLoaded)
ECS_NO_ORDER
static void set_envi_cover_params_es(const ecs::Event &,
  const Point4 &envi_cover_intensity_map_left_top_right_bottom,
  const ecs::string &envi_cover_intensity_map,
  const Point4 &envi_cover_albedo,
  const Point4 &envi_cover_normal,
  float envi_cover_reflectance,
  float envi_cover_translucency,
  float envi_cover_smoothness,
  float envi_cover_normal_infl,
  float envi_cover_depth_smoothstep_max,
  float envi_cover_depth_pow_exponent,
  float envi_cover_noise_high_frequency,
  float envi_cover_noise_low_frequency,
  float envi_cover_noise_mask_factor,
  float envi_cover_depth_mask_threshold,
  float envi_cover_normal_mask_threshold,
  float envi_cover_depth_mask_contrast,
  float envi_cover_normal_mask_contrast,
  float envi_cover_lowest_intensity)
{
  envi_cover_vars::set_params(envi_cover_intensity_map_left_top_right_bottom, envi_cover_intensity_map.c_str(), envi_cover_albedo,
    envi_cover_normal, envi_cover_reflectance, envi_cover_translucency, envi_cover_smoothness, envi_cover_normal_infl,
    envi_cover_depth_smoothstep_max, envi_cover_depth_pow_exponent, envi_cover_noise_high_frequency, envi_cover_noise_low_frequency,
    envi_cover_noise_mask_factor, envi_cover_depth_mask_threshold, envi_cover_normal_mask_threshold, envi_cover_depth_mask_contrast,
    envi_cover_normal_mask_contrast, envi_cover_lowest_intensity);
}