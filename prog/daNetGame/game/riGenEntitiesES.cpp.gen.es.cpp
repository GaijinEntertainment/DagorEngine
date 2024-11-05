#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t ri_extra__handle_get_type();
static ecs::LTComponentList ri_extra__handle_component(ECS_HASH("ri_extra__handle"), ri_extra__handle_get_type(), "prog/daNetGame/game/riGenEntitiesES.cpp.inl", "", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGame/game/riGenEntitiesES.cpp.inl", "", 0);
#include "riGenEntitiesES.cpp.inl"
ECS_DEF_PULL_VAR(riGenEntities);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc ri_extra_gen_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ri_extra_gen"), ecs::ComponentTypeInfo<RiExtraGen>()},
//start of 2 ro components at [1]
  {ECS_HASH("ri_extra_gen__riName"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("ri_extra_gen__template"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void ri_extra_gen_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventLevelLoaded>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ri_extra_gen_es_event_handler(static_cast<const EventLevelLoaded&>(evt)
        , ECS_RW_COMP(ri_extra_gen_es_event_handler_comps, "ri_extra_gen", RiExtraGen)
    , ECS_RO_COMP(ri_extra_gen_es_event_handler_comps, "ri_extra_gen__riName", ecs::string)
    , ECS_RO_COMP(ri_extra_gen_es_event_handler_comps, "ri_extra_gen__template", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ri_extra_gen_es_event_handler_es_desc
(
  "ri_extra_gen_es",
  "prog/daNetGame/game/riGenEntitiesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ri_extra_gen_es_event_handler_all_events),
  make_span(ri_extra_gen_es_event_handler_comps+0, 1)/*rw*/,
  make_span(ri_extra_gen_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded>::build(),
  0
,"server");
static constexpr ecs::ComponentDesc ri_extra_gen_mark_dynamic_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("ri_extra_gen__blk"), ecs::ComponentTypeInfo<ecs::string>()}
};
static void ri_extra_gen_mark_dynamic_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ri_extra_gen_mark_dynamic_es_event_handler(evt
        , ECS_RO_COMP(ri_extra_gen_mark_dynamic_es_event_handler_comps, "ri_extra_gen__blk", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ri_extra_gen_mark_dynamic_es_event_handler_es_desc
(
  "ri_extra_gen_mark_dynamic_es",
  "prog/daNetGame/game/riGenEntitiesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ri_extra_gen_mark_dynamic_es_event_handler_all_events),
  empty_span(),
  make_span(ri_extra_gen_mark_dynamic_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc ri_extra_gen_blk_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ri_extra_gen"), ecs::ComponentTypeInfo<RiExtraGen>()},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("ri_extra_gen__blk"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("ri_extra_gen__createEntityWhenDone"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL}
};
static void ri_extra_gen_blk_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ri_extra_gen_blk_es_event_handler(evt
        , ECS_RO_COMP(ri_extra_gen_blk_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(ri_extra_gen_blk_es_event_handler_comps, "ri_extra_gen", RiExtraGen)
    , ECS_RO_COMP(ri_extra_gen_blk_es_event_handler_comps, "ri_extra_gen__blk", ecs::string)
    , ECS_RO_COMP_PTR(ri_extra_gen_blk_es_event_handler_comps, "ri_extra_gen__createEntityWhenDone", ecs::string)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ri_extra_gen_blk_es_event_handler_es_desc
(
  "ri_extra_gen_blk_es",
  "prog/daNetGame/game/riGenEntitiesES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ri_extra_gen_blk_es_event_handler_all_events),
  make_span(ri_extra_gen_blk_es_event_handler_comps+0, 1)/*rw*/,
  make_span(ri_extra_gen_blk_es_event_handler_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded,
                       EventRIGenExtraRequested>::build(),
  0
,"server");
static constexpr ecs::ComponentDesc find_overall_game_zone_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("shrinkedZonePos"), ecs::ComponentTypeInfo<Point2>()},
  {ECS_HASH("shrinkZoneRadius"), ecs::ComponentTypeInfo<float>()},
//start of 1 rq components at [2]
  {ECS_HASH("disableExternalArea"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc find_overall_game_zone_ecs_query_desc
(
  "find_overall_game_zone_ecs_query",
  empty_span(),
  make_span(find_overall_game_zone_ecs_query_comps+0, 2)/*ro*/,
  make_span(find_overall_game_zone_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline ecs::QueryCbResult find_overall_game_zone_ecs_query(Callable function)
{
  return perform_query(g_entity_mgr, find_overall_game_zone_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(find_overall_game_zone_ecs_query_comps, "shrinkedZonePos", Point2)
            , ECS_RO_COMP(find_overall_game_zone_ecs_query_comps, "shrinkZoneRadius", float)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    }
  );
}
static constexpr ecs::component_t ri_extra__handle_get_type(){return ecs::ComponentTypeInfo<uint64_t>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
