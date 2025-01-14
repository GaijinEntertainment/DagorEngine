#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t effect__name_get_type();
static ecs::LTComponentList effect__name_component(ECS_HASH("effect__name"), effect__name_get_type(), "prog/daNetGame/main/levelES.cpp.inl", "", 0);
static constexpr ecs::component_t skies_settings__weatherSeed_get_type();
static ecs::LTComponentList skies_settings__weatherSeed_component(ECS_HASH("skies_settings__weatherSeed"), skies_settings__weatherSeed_get_type(), "prog/daNetGame/main/levelES.cpp.inl", "", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGame/main/levelES.cpp.inl", "", 0);
#include "levelES.cpp.inl"
ECS_DEF_PULL_VAR(level);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc level_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("level__loaded"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [1]
  {ECS_HASH("level"), ecs::ComponentTypeInfo<LocationHolder>()}
};
static void level_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventLevelLoaded>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    level_es(static_cast<const EventLevelLoaded&>(evt)
        , ECS_RW_COMP_PTR(level_es_comps, "level__loaded", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc level_es_es_desc
(
  "level_es",
  "prog/daNetGame/main/levelES.cpp.inl",
  ecs::EntitySystemOps(nullptr, level_es_all_events),
  make_span(level_es_comps+0, 1)/*rw*/,
  empty_span(),
  make_span(level_es_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded>::build(),
  0
);
static constexpr ecs::ComponentDesc world_renderer_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("world_renderer_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void world_renderer_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  world_renderer_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc world_renderer_es_event_handler_es_desc
(
  "world_renderer_es",
  "prog/daNetGame/main/levelES.cpp.inl",
  ecs::EntitySystemOps(nullptr, world_renderer_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(world_renderer_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc level_is_loading_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("level_is_loading"), ecs::ComponentTypeInfo<bool>()}
};
static ecs::CompileTimeQueryDesc level_is_loading_ecs_query_desc
(
  "level_is_loading_ecs_query",
  make_span(level_is_loading_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void level_is_loading_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, level_is_loading_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(level_is_loading_ecs_query_comps, "level_is_loading", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc get_level_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("level"), ecs::ComponentTypeInfo<LocationHolder>()}
};
static ecs::CompileTimeQueryDesc get_level_ecs_query_desc
(
  "get_level_ecs_query",
  empty_span(),
  make_span(get_level_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_level_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_level_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_level_ecs_query_comps, "level", LocationHolder)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc delete_weather_choice_entities_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("weather_choice_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc delete_weather_choice_entities_ecs_query_desc
(
  "delete_weather_choice_entities_ecs_query",
  empty_span(),
  make_span(delete_weather_choice_entities_ecs_query_comps+0, 1)/*ro*/,
  make_span(delete_weather_choice_entities_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void delete_weather_choice_entities_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, delete_weather_choice_entities_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(delete_weather_choice_entities_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc query_get_level_weather_and_time_seeds_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("level__weatherSeed"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("level__timeSeed"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc query_get_level_weather_and_time_seeds_ecs_query_desc
(
  "query_get_level_weather_and_time_seeds_ecs_query",
  empty_span(),
  make_span(query_get_level_weather_and_time_seeds_ecs_query_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void query_get_level_weather_and_time_seeds_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, query_get_level_weather_and_time_seeds_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(query_get_level_weather_and_time_seeds_ecs_query_comps, "level__weatherSeed", int)
            , ECS_RO_COMP(query_get_level_weather_and_time_seeds_ecs_query_comps, "level__timeSeed", int)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc query_get_skies_seed_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("skies_settings__weatherSeed"), ecs::ComponentTypeInfo<int>()}
};
static ecs::CompileTimeQueryDesc query_get_skies_seed_ecs_query_desc
(
  "query_get_skies_seed_ecs_query",
  empty_span(),
  make_span(query_get_skies_seed_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void query_get_skies_seed_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, query_get_skies_seed_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(query_get_skies_seed_ecs_query_comps, "skies_settings__weatherSeed", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc query_level_entity_eid_set_seed_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("level__weatherSeed"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("level__timeSeed"), ecs::ComponentTypeInfo<int>()},
//start of 1 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc query_level_entity_eid_set_seed_ecs_query_desc
(
  "query_level_entity_eid_set_seed_ecs_query",
  make_span(query_level_entity_eid_set_seed_ecs_query_comps+0, 2)/*rw*/,
  make_span(query_level_entity_eid_set_seed_ecs_query_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void query_level_entity_eid_set_seed_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, query_level_entity_eid_set_seed_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(query_level_entity_eid_set_seed_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(query_level_entity_eid_set_seed_ecs_query_comps, "level__weatherSeed", int)
            , ECS_RW_COMP(query_level_entity_eid_set_seed_ecs_query_comps, "level__timeSeed", int)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc query_get_level_seeds_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("level__weatherSeed"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("level__timeSeed"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("level__timeOfDay"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("level__weather"), ecs::ComponentTypeInfo<ecs::string>()}
};
static ecs::CompileTimeQueryDesc query_get_level_seeds_ecs_query_desc
(
  "query_get_level_seeds_ecs_query",
  empty_span(),
  make_span(query_get_level_seeds_ecs_query_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void query_get_level_seeds_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, query_get_level_seeds_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(query_get_level_seeds_ecs_query_comps, "level__weatherSeed", int)
            , ECS_RO_COMP(query_get_level_seeds_ecs_query_comps, "level__timeSeed", int)
            , ECS_RO_COMP(query_get_level_seeds_ecs_query_comps, "level__timeOfDay", float)
            , ECS_RO_COMP(query_get_level_seeds_ecs_query_comps, "level__weather", ecs::string)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc query_weather_entity_for_export_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("weather_choice_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc query_weather_entity_for_export_ecs_query_desc
(
  "query_weather_entity_for_export_ecs_query",
  empty_span(),
  make_span(query_weather_entity_for_export_ecs_query_comps+0, 1)/*ro*/,
  make_span(query_weather_entity_for_export_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void query_weather_entity_for_export_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, query_weather_entity_for_export_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(query_weather_entity_for_export_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t effect__name_get_type(){return ecs::ComponentTypeInfo<eastl::basic_string<char, eastl::allocator>>::type; }
static constexpr ecs::component_t skies_settings__weatherSeed_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
