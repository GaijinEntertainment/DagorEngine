// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "giObjectsES.cpp.inl"
ECS_DEF_PULL_VAR(giObjects);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc gi_objects_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("game_objects"), ecs::ComponentTypeInfo<GameObjects>()}
};
static void gi_objects_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
if (evt.is<EventGameObjectsCreated>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      gi_objects_es_event_handler(static_cast<const EventGameObjectsCreated&>(evt)
            , ECS_RW_COMP(gi_objects_es_event_handler_comps, "game_objects", GameObjects)
      );
    while (++comp != compE);
  } else {
    gi_objects_es_event_handler(evt
            );
}
}
static ecs::EntitySystemDesc gi_objects_es_event_handler_es_desc
(
  "gi_objects_es",
  "prog/daNetGame/render/giObjectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gi_objects_es_event_handler_all_events),
  make_span(gi_objects_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventGameObjectsCreated,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc gi_ignore_static_shadows_on_appear_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("gi_ignore_static_shadows"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void gi_ignore_static_shadows_on_appear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  gi_ignore_static_shadows_on_appear_es(evt
        );
}
static ecs::EntitySystemDesc gi_ignore_static_shadows_on_appear_es_es_desc
(
  "gi_ignore_static_shadows_on_appear_es",
  "prog/daNetGame/render/giObjectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gi_ignore_static_shadows_on_appear_es_all_events),
  empty_span(),
  empty_span(),
  make_span(gi_ignore_static_shadows_on_appear_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"render");
static constexpr ecs::ComponentDesc gi_ignore_static_shadows_on_disappear_es_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("gi_ignore_static_shadows"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void gi_ignore_static_shadows_on_disappear_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  gi_ignore_static_shadows_on_disappear_es(evt
        );
}
static ecs::EntitySystemDesc gi_ignore_static_shadows_on_disappear_es_es_desc
(
  "gi_ignore_static_shadows_on_disappear_es",
  "prog/daNetGame/render/giObjectsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gi_ignore_static_shadows_on_disappear_es_all_events),
  empty_span(),
  empty_span(),
  make_span(gi_ignore_static_shadows_on_disappear_es_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"render");
