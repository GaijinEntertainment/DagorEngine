// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "decalsES.cpp.inl"
ECS_DEF_PULL_VAR(decals);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc decals_es_event_handler_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("decals__useDecalMatrices"), ecs::ComponentTypeInfo<bool>()}
};
static void decals_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(decals_es_event_handler_comps, "decals__useDecalMatrices", bool)) )
      continue;
    decals_es_event_handler(evt
          , ECS_RO_COMP(decals_es_event_handler_comps, "eid", ecs::EntityId)
      , ECS_RO_COMP(decals_es_event_handler_comps, "transform", TMatrix)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc decals_es_event_handler_es_desc
(
  "decals_es",
  "prog/gameLibs/ecs/render/decalsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, decals_es_event_handler_all_events),
  empty_span(),
  make_span(decals_es_event_handler_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"transform");
static constexpr ecs::ComponentDesc decals_delete_es_event_handler_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("decals__useDecalMatrices"), ecs::ComponentTypeInfo<bool>()}
};
static void decals_delete_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(decals_delete_es_event_handler_comps, "decals__useDecalMatrices", bool)) )
      continue;
    decals_delete_es_event_handler(evt
          , ECS_RO_COMP(decals_delete_es_event_handler_comps, "eid", ecs::EntityId)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc decals_delete_es_event_handler_es_desc
(
  "decals_delete_es",
  "prog/gameLibs/ecs/render/decalsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, decals_delete_es_event_handler_all_events),
  empty_span(),
  make_span(decals_delete_es_event_handler_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
