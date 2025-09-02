// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "rumbleES.cpp.inl"
ECS_DEF_PULL_VAR(rumble);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc process_rumble_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("human_input__rumbleEvents"), ecs::ComponentTypeInfo<ecs::Object>()},
//start of 1 rq components at [1]
  {ECS_HASH("human_input__rumbleEnabled"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void process_rumble_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<rumble::CmdRumble>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    process_rumble_es_event_handler(static_cast<const rumble::CmdRumble&>(evt)
        , ECS_RO_COMP(process_rumble_es_event_handler_comps, "human_input__rumbleEvents", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc process_rumble_es_event_handler_es_desc
(
  "process_rumble_es",
  "prog/gameLibs/ecs/input/rumbleES.cpp.inl",
  ecs::EntitySystemOps(nullptr, process_rumble_es_event_handler_all_events),
  empty_span(),
  make_span(process_rumble_es_event_handler_comps+0, 1)/*ro*/,
  make_span(process_rumble_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<rumble::CmdRumble>::build(),
  0
,"input");
static constexpr ecs::ComponentDesc process_scaled_rumble_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("human_input__rumbleEvents"), ecs::ComponentTypeInfo<ecs::Object>()},
//start of 1 rq components at [1]
  {ECS_HASH("human_input__rumbleEnabled"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void process_scaled_rumble_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<rumble::CmdScaledRumble>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    process_scaled_rumble_es_event_handler(static_cast<const rumble::CmdScaledRumble&>(evt)
        , ECS_RO_COMP(process_scaled_rumble_es_event_handler_comps, "human_input__rumbleEvents", ecs::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc process_scaled_rumble_es_event_handler_es_desc
(
  "process_scaled_rumble_es",
  "prog/gameLibs/ecs/input/rumbleES.cpp.inl",
  ecs::EntitySystemOps(nullptr, process_scaled_rumble_es_event_handler_all_events),
  empty_span(),
  make_span(process_scaled_rumble_es_event_handler_comps+0, 1)/*ro*/,
  make_span(process_scaled_rumble_es_event_handler_comps+1, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<rumble::CmdScaledRumble>::build(),
  0
,"input");
