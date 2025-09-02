// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "bloodFootprintES.cpp.inl"
ECS_DEF_PULL_VAR(bloodFootprint);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc foot_step_in_blood_puddles_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("blood_footprint_emitter"), ecs::ComponentTypeInfo<BloodFootprintEmitter>()}
};
static void foot_step_in_blood_puddles_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventBloodFootStep>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    foot_step_in_blood_puddles_es_event_handler(static_cast<const EventBloodFootStep&>(evt)
        , ECS_RW_COMP(foot_step_in_blood_puddles_es_event_handler_comps, "blood_footprint_emitter", BloodFootprintEmitter)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc foot_step_in_blood_puddles_es_event_handler_es_desc
(
  "foot_step_in_blood_puddles_es",
  "prog/daNetGameLibs/blood_puddles/private/render/bloodFootprintES.cpp.inl",
  ecs::EntitySystemOps(nullptr, foot_step_in_blood_puddles_es_event_handler_all_events),
  make_span(foot_step_in_blood_puddles_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventBloodFootStep>::build(),
  0
);
static constexpr ecs::ComponentDesc emit_blood_footprint_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("blood_footprint_emitter"), ecs::ComponentTypeInfo<BloodFootprintEmitter>()},
//start of 2 ro components at [1]
  {ECS_HASH("footprintType"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void emit_blood_footprint_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventBloodFootStep>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    emit_blood_footprint_es_event_handler(static_cast<const EventBloodFootStep&>(evt)
        , ECS_RW_COMP(emit_blood_footprint_es_event_handler_comps, "blood_footprint_emitter", BloodFootprintEmitter)
    , ECS_RO_COMP(emit_blood_footprint_es_event_handler_comps, "footprintType", int)
    , ECS_RO_COMP(emit_blood_footprint_es_event_handler_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc emit_blood_footprint_es_event_handler_es_desc
(
  "emit_blood_footprint_es",
  "prog/daNetGameLibs/blood_puddles/private/render/bloodFootprintES.cpp.inl",
  ecs::EntitySystemOps(nullptr, emit_blood_footprint_es_event_handler_all_events),
  make_span(emit_blood_footprint_es_event_handler_comps+0, 1)/*rw*/,
  make_span(emit_blood_footprint_es_event_handler_comps+1, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventBloodFootStep>::build(),
  0
);
