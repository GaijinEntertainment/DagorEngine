#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t burnt_ground_biome_query__adjusted_radius_get_type();
static ecs::LTComponentList burnt_ground_biome_query__adjusted_radius_component(ECS_HASH("burnt_ground_biome_query__adjusted_radius"), burnt_ground_biome_query__adjusted_radius_get_type(), "prog/daNetGameLibs/burnt_ground/render/burntGroundEventsES.cpp.inl", "", 0);
static constexpr ecs::component_t burnt_ground_biome_query__animated_get_type();
static ecs::LTComponentList burnt_ground_biome_query__animated_component(ECS_HASH("burnt_ground_biome_query__animated"), burnt_ground_biome_query__animated_get_type(), "prog/daNetGameLibs/burnt_ground/render/burntGroundEventsES.cpp.inl", "", 0);
static constexpr ecs::component_t burnt_ground_biome_query__attempts_remaining_get_type();
static ecs::LTComponentList burnt_ground_biome_query__attempts_remaining_component(ECS_HASH("burnt_ground_biome_query__attempts_remaining"), burnt_ground_biome_query__attempts_remaining_get_type(), "prog/daNetGameLibs/burnt_ground/render/burntGroundEventsES.cpp.inl", "", 0);
static constexpr ecs::component_t burnt_ground_biome_query__biome_query_id_get_type();
static ecs::LTComponentList burnt_ground_biome_query__biome_query_id_component(ECS_HASH("burnt_ground_biome_query__biome_query_id"), burnt_ground_biome_query__biome_query_id_get_type(), "prog/daNetGameLibs/burnt_ground/render/burntGroundEventsES.cpp.inl", "", 0);
static constexpr ecs::component_t burnt_ground_biome_query__ground_height_get_type();
static ecs::LTComponentList burnt_ground_biome_query__ground_height_component(ECS_HASH("burnt_ground_biome_query__ground_height"), burnt_ground_biome_query__ground_height_get_type(), "prog/daNetGameLibs/burnt_ground/render/burntGroundEventsES.cpp.inl", "", 0);
static constexpr ecs::component_t burnt_ground_biome_query__radius_get_type();
static ecs::LTComponentList burnt_ground_biome_query__radius_component(ECS_HASH("burnt_ground_biome_query__radius"), burnt_ground_biome_query__radius_get_type(), "prog/daNetGameLibs/burnt_ground/render/burntGroundEventsES.cpp.inl", "", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGameLibs/burnt_ground/render/burntGroundEventsES.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "burntGroundEventsES.cpp.inl"
ECS_DEF_PULL_VAR(burntGroundEvents);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc burnt_ground__on_appear_event_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("burnt_ground__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground__placement_time"), ecs::ComponentTypeInfo<float>()}
};
static void burnt_ground__on_appear_event_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    burnt_ground__on_appear_event_es(evt
        , ECS_RO_COMP(burnt_ground__on_appear_event_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(burnt_ground__on_appear_event_es_comps, "burnt_ground__radius", float)
    , ECS_RO_COMP(burnt_ground__on_appear_event_es_comps, "burnt_ground__placement_time", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc burnt_ground__on_appear_event_es_es_desc
(
  "burnt_ground__on_appear_event_es",
  "prog/daNetGameLibs/burnt_ground/render/burntGroundEventsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, burnt_ground__on_appear_event_es_all_events),
  empty_span(),
  make_span(burnt_ground__on_appear_event_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc burnt_ground_biome_query_update_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("burnt_ground_biome_query__biome_query_id"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("burnt_ground_biome_query__attempts_remaining"), ecs::ComponentTypeInfo<int>()},
//start of 6 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("burnt_ground_biome_query__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_biome_query__adjusted_radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_biome_query__ground_height"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("burnt_ground_biome_query__animated"), ecs::ComponentTypeInfo<bool>()}
};
static void burnt_ground_biome_query_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdateStageInfoBeforeRender>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    burnt_ground_biome_query_update_es(static_cast<const UpdateStageInfoBeforeRender&>(evt)
        , components.manager()
    , ECS_RO_COMP(burnt_ground_biome_query_update_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(burnt_ground_biome_query_update_es_comps, "transform", TMatrix)
    , ECS_RW_COMP(burnt_ground_biome_query_update_es_comps, "burnt_ground_biome_query__biome_query_id", int)
    , ECS_RO_COMP(burnt_ground_biome_query_update_es_comps, "burnt_ground_biome_query__radius", float)
    , ECS_RO_COMP(burnt_ground_biome_query_update_es_comps, "burnt_ground_biome_query__adjusted_radius", float)
    , ECS_RO_COMP(burnt_ground_biome_query_update_es_comps, "burnt_ground_biome_query__ground_height", float)
    , ECS_RW_COMP(burnt_ground_biome_query_update_es_comps, "burnt_ground_biome_query__attempts_remaining", int)
    , ECS_RO_COMP(burnt_ground_biome_query_update_es_comps, "burnt_ground_biome_query__animated", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc burnt_ground_biome_query_update_es_es_desc
(
  "burnt_ground_biome_query_update_es",
  "prog/daNetGameLibs/burnt_ground/render/burntGroundEventsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, burnt_ground_biome_query_update_es_all_events),
  make_span(burnt_ground_biome_query_update_es_comps+0, 2)/*rw*/,
  make_span(burnt_ground_biome_query_update_es_comps+2, 6)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdateStageInfoBeforeRender>::build(),
  0
,"render");
static constexpr ecs::component_t burnt_ground_biome_query__adjusted_radius_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t burnt_ground_biome_query__animated_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
static constexpr ecs::component_t burnt_ground_biome_query__attempts_remaining_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t burnt_ground_biome_query__biome_query_id_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t burnt_ground_biome_query__ground_height_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t burnt_ground_biome_query__radius_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
