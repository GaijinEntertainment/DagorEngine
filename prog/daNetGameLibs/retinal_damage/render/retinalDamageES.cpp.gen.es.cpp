// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "retinalDamageES.cpp.inl"
ECS_DEF_PULL_VAR(retinalDamage);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc retinal_damage_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("retinal_damage__explosion_world_position"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("retinal_damage__scale"), ecs::ComponentTypeInfo<float>()}
};
static void retinal_damage_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    retinal_damage_es_event_handler(evt
        , ECS_RO_COMP(retinal_damage_es_event_handler_comps, "retinal_damage__explosion_world_position", Point3)
    , ECS_RO_COMP(retinal_damage_es_event_handler_comps, "retinal_damage__scale", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc retinal_damage_es_event_handler_es_desc
(
  "retinal_damage_es",
  "prog/daNetGameLibs/retinal_damage/render/retinalDamageES.cpp.inl",
  ecs::EntitySystemOps(nullptr, retinal_damage_es_event_handler_all_events),
  empty_span(),
  make_span(retinal_damage_es_event_handler_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc retinal_damage_stop_es_event_handler_comps[] =
{
//start of 2 rq components at [0]
  {ECS_HASH("retinal_damage__explosion_world_position"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("retinal_damage__scale"), ecs::ComponentTypeInfo<float>()}
};
static void retinal_damage_stop_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  retinal_damage_stop_es_event_handler(evt
        );
}
static ecs::EntitySystemDesc retinal_damage_stop_es_event_handler_es_desc
(
  "retinal_damage_stop_es",
  "prog/daNetGameLibs/retinal_damage/render/retinalDamageES.cpp.inl",
  ecs::EntitySystemOps(nullptr, retinal_damage_stop_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(retinal_damage_stop_es_event_handler_comps+0, 2)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc start_retinal_damage_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("retinal_damage__remaining_time"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("retinal_damage__node_handle"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("retinal_damage__tex"), ecs::ComponentTypeInfo<UniqueTexHolder>()},
//start of 3 ro components at [3]
  {ECS_HASH("retinal_damage__total_lifetime"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("retinal_damage__base_scale"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("retinal_damage__scale_increase_rate"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc start_retinal_damage_ecs_query_desc
(
  "start_retinal_damage_ecs_query",
  make_span(start_retinal_damage_ecs_query_comps+0, 3)/*rw*/,
  make_span(start_retinal_damage_ecs_query_comps+3, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void start_retinal_damage_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, start_retinal_damage_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(start_retinal_damage_ecs_query_comps, "retinal_damage__total_lifetime", float)
            , ECS_RO_COMP(start_retinal_damage_ecs_query_comps, "retinal_damage__base_scale", float)
            , ECS_RO_COMP(start_retinal_damage_ecs_query_comps, "retinal_damage__scale_increase_rate", float)
            , ECS_RW_COMP(start_retinal_damage_ecs_query_comps, "retinal_damage__remaining_time", float)
            , ECS_RW_COMP(start_retinal_damage_ecs_query_comps, "retinal_damage__node_handle", dafg::NodeHandle)
            , ECS_RW_COMP(start_retinal_damage_ecs_query_comps, "retinal_damage__tex", UniqueTexHolder)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc stop_retinal_damage_ecs_query_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("retinal_damage__node_handle"), ecs::ComponentTypeInfo<dafg::NodeHandle>()},
  {ECS_HASH("retinal_damage__tex"), ecs::ComponentTypeInfo<UniqueTexHolder>()},
  {ECS_HASH("retinal_damage__remaining_time"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc stop_retinal_damage_ecs_query_desc
(
  "stop_retinal_damage_ecs_query",
  make_span(stop_retinal_damage_ecs_query_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void stop_retinal_damage_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, stop_retinal_damage_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RW_COMP(stop_retinal_damage_ecs_query_comps, "retinal_damage__node_handle", dafg::NodeHandle)
            , ECS_RW_COMP(stop_retinal_damage_ecs_query_comps, "retinal_damage__tex", UniqueTexHolder)
            , ECS_RW_COMP(stop_retinal_damage_ecs_query_comps, "retinal_damage__remaining_time", float)
            );

        }while (++comp != compE);
    }
  );
}
