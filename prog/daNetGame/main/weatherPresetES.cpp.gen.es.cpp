// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "weatherPresetES.cpp.inl"
ECS_DEF_PULL_VAR(weatherPreset);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc query_rain_entity_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("rain_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc query_rain_entity_ecs_query_desc
(
  "query_rain_entity_ecs_query",
  empty_span(),
  make_span(query_rain_entity_ecs_query_comps+0, 1)/*ro*/,
  make_span(query_rain_entity_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void query_rain_entity_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, query_rain_entity_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(query_rain_entity_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc query_snow_entity_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("snow_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc query_snow_entity_ecs_query_desc
(
  "query_snow_entity_ecs_query",
  empty_span(),
  make_span(query_snow_entity_ecs_query_comps+0, 1)/*ro*/,
  make_span(query_snow_entity_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void query_snow_entity_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, query_snow_entity_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(query_snow_entity_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc query_lightning_entity_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("lightning_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc query_lightning_entity_ecs_query_desc
(
  "query_lightning_entity_ecs_query",
  empty_span(),
  make_span(query_lightning_entity_ecs_query_comps+0, 1)/*ro*/,
  make_span(query_lightning_entity_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void query_lightning_entity_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, query_lightning_entity_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(query_lightning_entity_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc delete_rain_entities_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("rain_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc delete_rain_entities_ecs_query_desc
(
  "delete_rain_entities_ecs_query",
  empty_span(),
  make_span(delete_rain_entities_ecs_query_comps+0, 1)/*ro*/,
  make_span(delete_rain_entities_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void delete_rain_entities_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, delete_rain_entities_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(delete_rain_entities_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc delete_snow_entities_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("snow_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc delete_snow_entities_ecs_query_desc
(
  "delete_snow_entities_ecs_query",
  empty_span(),
  make_span(delete_snow_entities_ecs_query_comps+0, 1)/*ro*/,
  make_span(delete_snow_entities_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void delete_snow_entities_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, delete_snow_entities_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(delete_snow_entities_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc delete_lightning_entities_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("lightning_tag"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc delete_lightning_entities_ecs_query_desc
(
  "delete_lightning_entities_ecs_query",
  empty_span(),
  make_span(delete_lightning_entities_ecs_query_comps+0, 1)/*ro*/,
  make_span(delete_lightning_entities_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void delete_lightning_entities_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, delete_lightning_entities_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(delete_lightning_entities_ecs_query_comps, "eid", ecs::EntityId)
            );

        }while (++comp != compE);
    }
  );
}
