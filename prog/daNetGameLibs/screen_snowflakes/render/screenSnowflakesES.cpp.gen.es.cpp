#include "screenSnowflakesES.cpp.inl"
ECS_DEF_PULL_VAR(screenSnowflakes);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc create_screen_snowflakes_renderer_entity_on_settings_changed_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__screenSpaceWeatherEffects"), ecs::ComponentTypeInfo<bool>()}
};
static void create_screen_snowflakes_renderer_entity_on_settings_changed_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    create_screen_snowflakes_renderer_entity_on_settings_changed_es(evt
        , ECS_RO_COMP(create_screen_snowflakes_renderer_entity_on_settings_changed_es_comps, "render_settings__screenSpaceWeatherEffects", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc create_screen_snowflakes_renderer_entity_on_settings_changed_es_es_desc
(
  "create_screen_snowflakes_renderer_entity_on_settings_changed_es",
  "prog/daNetGameLibs/screen_snowflakes/render/screenSnowflakesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_screen_snowflakes_renderer_entity_on_settings_changed_es_all_events),
  empty_span(),
  make_span(create_screen_snowflakes_renderer_entity_on_settings_changed_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ChangeRenderFeatures,
                       OnRenderSettingsReady,
                       SetResolutionEvent>::build(),
  0
,"render","render_settings__screenSpaceWeatherEffects");
static constexpr ecs::ComponentDesc create_screen_snowflakes_renderer_entity_on_snow_appearance_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("snow_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void create_screen_snowflakes_renderer_entity_on_snow_appearance_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  create_screen_snowflakes_renderer_entity_on_snow_appearance_es(evt
        );
}
static ecs::EntitySystemDesc create_screen_snowflakes_renderer_entity_on_snow_appearance_es_es_desc
(
  "create_screen_snowflakes_renderer_entity_on_snow_appearance_es",
  "prog/daNetGameLibs/screen_snowflakes/render/screenSnowflakesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, create_screen_snowflakes_renderer_entity_on_snow_appearance_es_all_events),
  empty_span(),
  empty_span(),
  make_span(create_screen_snowflakes_renderer_entity_on_snow_appearance_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc destroy_screen_snowflakes_renderer_entity_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("snow_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void destroy_screen_snowflakes_renderer_entity_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  destroy_screen_snowflakes_renderer_entity_es(evt
        );
}
static ecs::EntitySystemDesc destroy_screen_snowflakes_renderer_entity_es_es_desc
(
  "destroy_screen_snowflakes_renderer_entity_es",
  "prog/daNetGameLibs/screen_snowflakes/render/screenSnowflakesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destroy_screen_snowflakes_renderer_entity_es_all_events),
  empty_span(),
  empty_span(),
  make_span(destroy_screen_snowflakes_renderer_entity_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc init_screen_snowflakes_es_comps[] =
{
//start of 4 rw components at [0]
  {ECS_HASH("screen_snowflakes__enabled_on_level"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("screen_snowflakes__camera_inside_vehicle"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("screen_snowflakes__instances_buf"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("screen_snowflakes__node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
//start of 1 ro components at [4]
  {ECS_HASH("screen_snowflakes__max_count"), ecs::ComponentTypeInfo<int>()}
};
static void init_screen_snowflakes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_screen_snowflakes_es(evt
        , ECS_RW_COMP(init_screen_snowflakes_es_comps, "screen_snowflakes__enabled_on_level", bool)
    , ECS_RW_COMP(init_screen_snowflakes_es_comps, "screen_snowflakes__camera_inside_vehicle", bool)
    , ECS_RO_COMP(init_screen_snowflakes_es_comps, "screen_snowflakes__max_count", int)
    , ECS_RW_COMP(init_screen_snowflakes_es_comps, "screen_snowflakes__instances_buf", UniqueBufHolder)
    , ECS_RW_COMP(init_screen_snowflakes_es_comps, "screen_snowflakes__node", dabfg::NodeHandle)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_screen_snowflakes_es_es_desc
(
  "init_screen_snowflakes_es",
  "prog/daNetGameLibs/screen_snowflakes/render/screenSnowflakesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_screen_snowflakes_es_all_events),
  make_span(init_screen_snowflakes_es_comps+0, 4)/*rw*/,
  make_span(init_screen_snowflakes_es_comps+4, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc destroy_screen_snowflakes_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("screen_snowflakes__instances_buf"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("screen_snowflakes__node"), ecs::ComponentTypeInfo<dabfg::NodeHandle>()},
  {ECS_HASH("screen_snowflakes__enabled_on_level"), ecs::ComponentTypeInfo<bool>()}
};
static void destroy_screen_snowflakes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    destroy_screen_snowflakes_es(evt
        , ECS_RW_COMP(destroy_screen_snowflakes_es_comps, "screen_snowflakes__instances_buf", UniqueBufHolder)
    , ECS_RW_COMP(destroy_screen_snowflakes_es_comps, "screen_snowflakes__node", dabfg::NodeHandle)
    , ECS_RW_COMP(destroy_screen_snowflakes_es_comps, "screen_snowflakes__enabled_on_level", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc destroy_screen_snowflakes_es_es_desc
(
  "destroy_screen_snowflakes_es",
  "prog/daNetGameLibs/screen_snowflakes/render/screenSnowflakesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, destroy_screen_snowflakes_es_all_events),
  make_span(destroy_screen_snowflakes_es_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc screen_snowflakes_on_vehicle_camera_change_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("isInVehicle"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [1]
  {ECS_HASH("bindedCamera"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void screen_snowflakes_on_vehicle_camera_change_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    screen_snowflakes_on_vehicle_camera_change_es(evt
        , ECS_RO_COMP(screen_snowflakes_on_vehicle_camera_change_es_comps, "isInVehicle", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc screen_snowflakes_on_vehicle_camera_change_es_es_desc
(
  "screen_snowflakes_on_vehicle_camera_change_es",
  "prog/daNetGameLibs/screen_snowflakes/render/screenSnowflakesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, screen_snowflakes_on_vehicle_camera_change_es_all_events),
  empty_span(),
  make_span(screen_snowflakes_on_vehicle_camera_change_es_comps+0, 1)/*ro*/,
  make_span(screen_snowflakes_on_vehicle_camera_change_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"bindedCamera,isInVehicle");
static constexpr ecs::ComponentDesc screen_snowflakes_before_render_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("screen_snowflakes__instances_buf"), ecs::ComponentTypeInfo<UniqueBufHolder>()},
  {ECS_HASH("screen_snowflakes__time_until_next_spawn"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_snowflakes__instances"), ecs::ComponentTypeInfo<SnowflakeInstances>()},
//start of 2 ro components at [3]
  {ECS_HASH("screen_snowflakes__camera_inside_vehicle"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("screen_snowflakes__enabled_on_level"), ecs::ComponentTypeInfo<bool>()}
};
static void screen_snowflakes_before_render_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(screen_snowflakes_before_render_es_comps, "screen_snowflakes__enabled_on_level", bool)) )
      continue;
    screen_snowflakes_before_render_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
          , ECS_RW_COMP(screen_snowflakes_before_render_es_comps, "screen_snowflakes__instances_buf", UniqueBufHolder)
      , ECS_RW_COMP(screen_snowflakes_before_render_es_comps, "screen_snowflakes__time_until_next_spawn", float)
      , ECS_RW_COMP(screen_snowflakes_before_render_es_comps, "screen_snowflakes__instances", SnowflakeInstances)
      , ECS_RO_COMP(screen_snowflakes_before_render_es_comps, "screen_snowflakes__camera_inside_vehicle", bool)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc screen_snowflakes_before_render_es_es_desc
(
  "screen_snowflakes_before_render_es",
  "prog/daNetGameLibs/screen_snowflakes/render/screenSnowflakesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, screen_snowflakes_before_render_es_all_events),
  make_span(screen_snowflakes_before_render_es_comps+0, 3)/*rw*/,
  make_span(screen_snowflakes_before_render_es_comps+3, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
//static constexpr ecs::ComponentDesc register_screen_snowflakes_for_postfx_es_comps[] ={};
static void register_screen_snowflakes_for_postfx_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<RegisterPostfxResources>());
  register_screen_snowflakes_for_postfx_es(static_cast<const RegisterPostfxResources&>(evt)
        );
}
static ecs::EntitySystemDesc register_screen_snowflakes_for_postfx_es_es_desc
(
  "register_screen_snowflakes_for_postfx_es",
  "prog/daNetGameLibs/screen_snowflakes/render/screenSnowflakesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_screen_snowflakes_for_postfx_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<RegisterPostfxResources>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc snow_enabled_on_level_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("snow_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc snow_enabled_on_level_ecs_query_desc
(
  "snow_enabled_on_level_ecs_query",
  empty_span(),
  make_span(snow_enabled_on_level_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void snow_enabled_on_level_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, snow_enabled_on_level_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(snow_enabled_on_level_ecs_query_comps, "snow_tag", ecs::Tag)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc snowflakes_enabled_global_setting_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("render_settings__screenSpaceWeatherEffects"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc snowflakes_enabled_global_setting_ecs_query_desc
(
  "snowflakes_enabled_global_setting_ecs_query",
  empty_span(),
  make_span(snowflakes_enabled_global_setting_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void snowflakes_enabled_global_setting_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, snowflakes_enabled_global_setting_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(snowflakes_enabled_global_setting_ecs_query_comps, "render_settings__screenSpaceWeatherEffects", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc vehicle_camera_on_screen_snowflakes_init_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("isInVehicle"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [1]
  {ECS_HASH("bindedCamera"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc vehicle_camera_on_screen_snowflakes_init_ecs_query_desc
(
  "vehicle_camera_on_screen_snowflakes_init_ecs_query",
  empty_span(),
  make_span(vehicle_camera_on_screen_snowflakes_init_ecs_query_comps+0, 1)/*ro*/,
  make_span(vehicle_camera_on_screen_snowflakes_init_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void vehicle_camera_on_screen_snowflakes_init_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, vehicle_camera_on_screen_snowflakes_init_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(vehicle_camera_on_screen_snowflakes_init_ecs_query_comps, "isInVehicle", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc render_screen_snowflakes_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("screen_snowflakes__instances"), ecs::ComponentTypeInfo<SnowflakeInstances>()}
};
static ecs::CompileTimeQueryDesc render_screen_snowflakes_ecs_query_desc
(
  "render_screen_snowflakes_ecs_query",
  empty_span(),
  make_span(render_screen_snowflakes_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void render_screen_snowflakes_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, render_screen_snowflakes_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(render_screen_snowflakes_ecs_query_comps, "screen_snowflakes__instances", SnowflakeInstances)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc screen_snowflakes_on_vehicle_camera_change_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("screen_snowflakes__camera_inside_vehicle"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [1]
  {ECS_HASH("screen_snowflakes__enabled_on_level"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc screen_snowflakes_on_vehicle_camera_change_ecs_query_desc
(
  "screen_snowflakes_on_vehicle_camera_change_ecs_query",
  make_span(screen_snowflakes_on_vehicle_camera_change_ecs_query_comps+0, 1)/*rw*/,
  make_span(screen_snowflakes_on_vehicle_camera_change_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void screen_snowflakes_on_vehicle_camera_change_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, screen_snowflakes_on_vehicle_camera_change_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(ECS_RO_COMP(screen_snowflakes_on_vehicle_camera_change_ecs_query_comps, "screen_snowflakes__enabled_on_level", bool)) )
            continue;
          function(
              ECS_RW_COMP(screen_snowflakes_on_vehicle_camera_change_ecs_query_comps, "screen_snowflakes__camera_inside_vehicle", bool)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::ComponentDesc screen_snowflakes_before_render_ecs_query_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("screen_snowflakes__spawn_rate"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_snowflakes__min_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_snowflakes__max_size"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_snowflakes__restricted_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("screen_snowflakes__lifetime"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc screen_snowflakes_before_render_ecs_query_desc
(
  "screen_snowflakes_before_render_ecs_query",
  empty_span(),
  make_span(screen_snowflakes_before_render_ecs_query_comps+0, 5)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void screen_snowflakes_before_render_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, screen_snowflakes_before_render_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(screen_snowflakes_before_render_ecs_query_comps, "screen_snowflakes__spawn_rate", float)
            , ECS_RO_COMP(screen_snowflakes_before_render_ecs_query_comps, "screen_snowflakes__min_size", float)
            , ECS_RO_COMP(screen_snowflakes_before_render_ecs_query_comps, "screen_snowflakes__max_size", float)
            , ECS_RO_COMP(screen_snowflakes_before_render_ecs_query_comps, "screen_snowflakes__restricted_radius", float)
            , ECS_RO_COMP(screen_snowflakes_before_render_ecs_query_comps, "screen_snowflakes__lifetime", float)
            );

        }
    }
  );
}
