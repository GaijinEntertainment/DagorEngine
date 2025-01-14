// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <3d/dag_quadIndexBuffer.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <drv/3d/dag_draw.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/render/resPtr.h>
#include <gamePhys/collision/collisionLib.h>
#include <math/random/dag_random.h>
#include <render/daBfg/ecs/frameGraphNode.h>
#include <render/renderEvent.h>
#include <render/renderSettings.h>
#include <shaders/dag_DynamicShaderHelper.h>

#include <math/dag_hlsl_floatx.h>
#include <screen_snowflakes/shaders/snowflake.hlsli>

using SnowflakeInstances = dag::Vector<Snowflake>;

ECS_DECLARE_RELOCATABLE_TYPE(SnowflakeInstances);
ECS_REGISTER_RELOCATABLE_TYPE(SnowflakeInstances, nullptr);

#define SCREEN_SNOWFLAKES_VARS VAR(screen_snowflakes_rendered)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
SCREEN_SNOWFLAKES_VARS
#undef VAR

static void create_or_destroy_screen_snowflakes_renderer_entity(bool render_snowflakes)
{
  if (render_snowflakes && renderer_has_feature(FeatureRenderFlags::POSTFX) &&
      g_entity_mgr->getTemplateDB().getTemplateByName("screen_snowflakes_renderer"))
    g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("screen_snowflakes_renderer"));
  else
    g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("screen_snowflakes_renderer")));
}

template <typename Callable>
static void snow_enabled_on_level_ecs_query(Callable c);

template <typename Callable>
static void snowflakes_enabled_global_setting_ecs_query(Callable c);

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady, SetResolutionEvent, ChangeRenderFeatures)
ECS_TRACK(render_settings__screenSpaceWeatherEffects)
static void create_screen_snowflakes_renderer_entity_on_settings_changed_es(const ecs::Event &,
  bool render_settings__screenSpaceWeatherEffects)
{
  bool renderSnowflakes = false;
  snow_enabled_on_level_ecs_query([&renderSnowflakes, render_settings__screenSpaceWeatherEffects](ecs::Tag snow_tag) {
    G_UNUSED(snow_tag);
    renderSnowflakes = render_settings__screenSpaceWeatherEffects;
  });
  create_or_destroy_screen_snowflakes_renderer_entity(renderSnowflakes);
}

ECS_TAG(render)
ECS_REQUIRE(ecs::Tag snow_tag)
ECS_ON_EVENT(on_appear)
static void create_screen_snowflakes_renderer_entity_on_snow_appearance_es(const ecs::Event &)
{
  bool renderSnowflakes = false;
  snowflakes_enabled_global_setting_ecs_query([&renderSnowflakes](bool render_settings__screenSpaceWeatherEffects) {
    renderSnowflakes = render_settings__screenSpaceWeatherEffects;
  });
  create_or_destroy_screen_snowflakes_renderer_entity(renderSnowflakes);
}

ECS_TAG(render)
ECS_REQUIRE(ecs::Tag snow_tag)
ECS_ON_EVENT(on_disappear)
static void destroy_screen_snowflakes_renderer_entity_es(const ecs::Event &)
{
  create_or_destroy_screen_snowflakes_renderer_entity(false);
}

template <typename Callable>
static void render_screen_snowflakes_ecs_query(Callable c);

template <typename Callable>
static void vehicle_camera_on_screen_snowflakes_init_ecs_query(Callable c);

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void init_screen_snowflakes_es(const ecs::Event &,
  bool &screen_snowflakes__enabled_on_level,
  bool &screen_snowflakes__camera_inside_vehicle,
  int screen_snowflakes__max_count,
  UniqueBufHolder &screen_snowflakes__instances_buf,
  dabfg::NodeHandle &screen_snowflakes__node)
{
  screen_snowflakes__enabled_on_level = true;

  vehicle_camera_on_screen_snowflakes_init_ecs_query(
    [&screen_snowflakes__camera_inside_vehicle](ECS_REQUIRE(ecs::EntityId bindedCamera) bool isInVehicle) {
      screen_snowflakes__camera_inside_vehicle = isInVehicle;
    });

  screen_snowflakes__instances_buf = dag::create_sbuffer(sizeof(Snowflake), screen_snowflakes__max_count,
    SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES, 0, "screen_snowflakes_buf");

  screen_snowflakes__node = dabfg::register_node("screen_snowflakes_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto screenSnowflakesTexHndl = registry.createTexture2d("screen_snowflakes_tex", dabfg::History::No,
      {TEXFMT_R16F | TEXCF_RTARGET, registry.getResolution<2>("post_fx", 0.5f)});
    registry.requestRenderPass().clear(screenSnowflakesTexHndl, make_clear_value(0, 0, 0, 0)).color({screenSnowflakesTexHndl});

    return [renderer = DynamicShaderHelper("screen_snowflakes")]() {
      render_screen_snowflakes_ecs_query([&renderer](const SnowflakeInstances &screen_snowflakes__instances) {
        if (screen_snowflakes__instances.empty())
          return;
        d3d::setvsrc_ex(0, NULL, 0, 0);
        index_buffer::use_quads_16bit();
        if (!renderer.shader->setStates())
          return;
        d3d::drawind_instanced(PRIM_TRILIST, 0, 2, 0, screen_snowflakes__instances.size(), 0);
      });
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static void destroy_screen_snowflakes_es(const ecs::Event &,
  UniqueBufHolder &screen_snowflakes__instances_buf,
  dabfg::NodeHandle &screen_snowflakes__node,
  bool &screen_snowflakes__enabled_on_level)
{
  screen_snowflakes__enabled_on_level = false;
  screen_snowflakes__instances_buf.close();
  screen_snowflakes__node = {};
  ShaderGlobal::set_int(screen_snowflakes_renderedVarId, 0);
}

template <typename Callable>
static void screen_snowflakes_on_vehicle_camera_change_ecs_query(Callable c);

ECS_TRACK(isInVehicle, bindedCamera)
ECS_REQUIRE(ecs::EntityId bindedCamera)
static void screen_snowflakes_on_vehicle_camera_change_es(const ecs::Event &, bool isInVehicle)
{
  screen_snowflakes_on_vehicle_camera_change_ecs_query(
    [isInVehicle](ECS_REQUIRE(eastl::true_type screen_snowflakes__enabled_on_level) bool &screen_snowflakes__camera_inside_vehicle) {
      screen_snowflakes__camera_inside_vehicle = isInVehicle;
    });
}

static float exponential_distribution(float u, float rate) { return -log(u) / rate; }

template <typename Callable>
static void screen_snowflakes_before_render_ecs_query(ecs::EntityId, Callable c);

ECS_TAG(render)
ECS_REQUIRE(eastl::true_type screen_snowflakes__enabled_on_level)
static void screen_snowflakes_before_render_es(const UpdateStageInfoBeforeRender &info,
  UniqueBufHolder &screen_snowflakes__instances_buf,
  float &screen_snowflakes__time_until_next_spawn,
  SnowflakeInstances &screen_snowflakes__instances,
  bool screen_snowflakes__camera_inside_vehicle)
{
  ecs::EntityId settingsEid = g_entity_mgr->getSingletonEntity(ECS_HASH("screen_snowflakes_settings"));
  if (!settingsEid)
    return;

  const Point3 SNOWFALL_DIRECTION(0, -1, 0);
  const Point3 &cameraPos = info.viewItm.getcol(3);
  float t = 25.0f;

  bool spawnSnowlakes = !screen_snowflakes__camera_inside_vehicle && !dacoll::rayhit_normalized_ri(cameraPos, -SNOWFALL_DIRECTION, t);
  screen_snowflakes__time_until_next_spawn -= info.dt;

  screen_snowflakes_before_render_ecs_query(settingsEid,
    [&](float screen_snowflakes__spawn_rate, float screen_snowflakes__min_size, float screen_snowflakes__max_size,
      float screen_snowflakes__restricted_radius, float screen_snowflakes__lifetime) {
      if (spawnSnowlakes && screen_snowflakes__time_until_next_spawn < 0.f)
      {
        float phi = rnd_float(0.f, 2 * M_PI);
        float cosPhi = cosf(phi);
        float sinPhi = sinf(phi);
        // R-coordinate of a square in polar coordinates
        float maxR = 1.f / max(abs(cosPhi), abs(sinPhi));
        // A random variable with the probability distribution function rising with the rising argument, which would guarantee a
        // higher probability of snowflake spawning near the edges of the screen rather than near the center:
        float rv = powf(rnd_float(0.f, 1.f), 0.33f);
        float r = screen_snowflakes__restricted_radius +
                  (maxR - screen_snowflakes__max_size / sqrtf(2.f) - screen_snowflakes__restricted_radius) * rv;
        Point2 snowflakeCenter = Point2(r * cosPhi, r * sinPhi);
        // The closer to the edge, the bigger the snowflake:
        float sizeX = screen_snowflakes__min_size + (screen_snowflakes__max_size - screen_snowflakes__min_size) * rv;
        sizeX *= 2.f;                               // due to screen space coordinates being from -1 to 1
        float sizeY = sizeX * rnd_float(0.8f, 1.f); // prefer horizontal snowflakes
        float opacity = rnd_float(0.6f, 1.f);
        screen_snowflakes__instances.push_back(Snowflake{snowflakeCenter, Point2(sizeX, sizeY), opacity, rnd_float(0.f, 1.e5f)});

        screen_snowflakes__time_until_next_spawn = exponential_distribution(rnd_float(1.e-3f, 1.f), screen_snowflakes__spawn_rate);
      }

      for (auto &snowflake : screen_snowflakes__instances)
        snowflake.opacity -= info.dt / screen_snowflakes__lifetime;
    });

  screen_snowflakes__instances.erase(eastl::remove_if(begin(screen_snowflakes__instances), end(screen_snowflakes__instances),
                                       [](const Snowflake &snowflake) { return snowflake.opacity < 0.f; }),
    end(screen_snowflakes__instances));

  if (!screen_snowflakes__instances.empty())
  {
    screen_snowflakes__instances_buf.getBuf()->updateData(0,
      screen_snowflakes__instances.size() * sizeof(screen_snowflakes__instances[0]), screen_snowflakes__instances.data(),
      VBLOCK_DISCARD);
    ShaderGlobal::set_int(screen_snowflakes_renderedVarId, 1);
  }
  else
    ShaderGlobal::set_int(screen_snowflakes_renderedVarId, 0);
}

ECS_TAG(render)
static void register_screen_snowflakes_for_postfx_es(const RegisterPostfxResources &evt)
{
  evt.get<0>().readTexture("screen_snowflakes_tex").atStage(dabfg::Stage::PS).bindToShaderVar().optional();
}