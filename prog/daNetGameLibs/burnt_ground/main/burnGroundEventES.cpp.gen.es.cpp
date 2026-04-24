#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t burnt_ground__placement_time_get_type();
static ecs::LTComponentList burnt_ground__placement_time_component(ECS_HASH("burnt_ground__placement_time"), burnt_ground__placement_time_get_type(), "prog/daNetGameLibs/burnt_ground/main/burnGroundEventES.cpp.inl", "burnt_ground__on_fire_appear_event_es", 0);
static constexpr ecs::component_t burnt_ground__radius_get_type();
static ecs::LTComponentList burnt_ground__radius_component(ECS_HASH("burnt_ground__radius"), burnt_ground__radius_get_type(), "prog/daNetGameLibs/burnt_ground/main/burnGroundEventES.cpp.inl", "burnt_ground__on_fire_appear_event_es", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGameLibs/burnt_ground/main/burnGroundEventES.cpp.inl", "burnt_ground__on_fire_appear_event_es", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "burnGroundEventES.cpp.inl"
ECS_DEF_PULL_VAR(burnGroundEvent);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc burnt_ground__on_fire_appear_event_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("burnt_ground__burn_radius"), ecs::ComponentTypeInfo<float>()}
};
static void burnt_ground__on_fire_appear_event_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    burnt_ground__on_fire_appear_event_es(evt
        , components.manager()
    , ECS_RO_COMP(burnt_ground__on_fire_appear_event_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(burnt_ground__on_fire_appear_event_es_comps, "burnt_ground__burn_radius", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc burnt_ground__on_fire_appear_event_es_es_desc
(
  "burnt_ground__on_fire_appear_event_es",
  "prog/daNetGameLibs/burnt_ground/main/burnGroundEventES.cpp.inl",
  ecs::EntitySystemOps(nullptr, burnt_ground__on_fire_appear_event_es_all_events),
  empty_span(),
  make_span(burnt_ground__on_fire_appear_event_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"server");
static constexpr ecs::component_t burnt_ground__placement_time_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t burnt_ground__radius_get_type(){return ecs::ComponentTypeInfo<float>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
