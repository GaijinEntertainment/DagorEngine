#include "restrictionBoxES.cpp.inl"
ECS_DEF_PULL_VAR(restrictionBox);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc restriction_box_objects_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("game_objects"), ecs::ComponentTypeInfo<GameObjects>()}
};
static void restriction_box_objects_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
if (evt.is<EventGameObjectsCreated>()) {
    auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
      restriction_box_objects_es_event_handler(static_cast<const EventGameObjectsCreated&>(evt)
            , ECS_RW_COMP(restriction_box_objects_es_event_handler_comps, "game_objects", GameObjects)
      );
    while (++comp != compE);
  } else {
    restriction_box_objects_es_event_handler(evt
            );
}
}
static ecs::EntitySystemDesc restriction_box_objects_es_event_handler_es_desc
(
  "restriction_box_objects_es",
  "prog/daNetGame/render/restrictionBoxES.cpp.inl",
  ecs::EntitySystemOps(nullptr, restriction_box_objects_es_event_handler_all_events),
  make_span(restriction_box_objects_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventGameObjectsCreated,
                       ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
