// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "joltVehicleKindES.cpp.inl"
ECS_DEF_PULL_VAR(joltVehicleKind);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc jolt_vehicle_step_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("phys_kind_net_phys"), ecs::ComponentTypeInfo<PhysKindActor>()},
  {ECS_HASH("jolt_vehicle"), ecs::ComponentTypeInfo<JoltVehicleKind>()},
//start of 2 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("jolt_vehicle__upright"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void jolt_vehicle_step_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<CmdPhysKindStep>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    jolt_vehicle_step_es_event_handler(static_cast<const CmdPhysKindStep&>(evt)
        , ECS_RO_COMP(jolt_vehicle_step_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(jolt_vehicle_step_es_event_handler_comps, "phys_kind_net_phys", PhysKindActor)
    , ECS_RW_COMP(jolt_vehicle_step_es_event_handler_comps, "jolt_vehicle", JoltVehicleKind)
    , ECS_RO_COMP_OR(jolt_vehicle_step_es_event_handler_comps, "jolt_vehicle__upright", bool(true))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc jolt_vehicle_step_es_event_handler_es_desc
(
  "jolt_vehicle_step_es",
  "prog/daNetGameLibs/phys_kind_jolt_vehicle/./joltVehicleKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, jolt_vehicle_step_es_event_handler_all_events),
  make_span(jolt_vehicle_step_es_event_handler_comps+0, 2)/*rw*/,
  make_span(jolt_vehicle_step_es_event_handler_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdPhysKindStep>::build(),
  0
);
static constexpr ecs::ComponentDesc jolt_vehicle_apply_state_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("jolt_vehicle"), ecs::ComponentTypeInfo<JoltVehicleKind>()},
//start of 1 ro components at [1]
  {ECS_HASH("phys_kind_net_phys"), ecs::ComponentTypeInfo<PhysKindActor>()}
};
static void jolt_vehicle_apply_state_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<CmdPhysKindApplyState>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    jolt_vehicle_apply_state_es_event_handler(static_cast<const CmdPhysKindApplyState&>(evt)
        , ECS_RO_COMP(jolt_vehicle_apply_state_es_event_handler_comps, "phys_kind_net_phys", PhysKindActor)
    , ECS_RW_COMP(jolt_vehicle_apply_state_es_event_handler_comps, "jolt_vehicle", JoltVehicleKind)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc jolt_vehicle_apply_state_es_event_handler_es_desc
(
  "jolt_vehicle_apply_state_es",
  "prog/daNetGameLibs/phys_kind_jolt_vehicle/./joltVehicleKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, jolt_vehicle_apply_state_es_event_handler_all_events),
  make_span(jolt_vehicle_apply_state_es_event_handler_comps+0, 1)/*rw*/,
  make_span(jolt_vehicle_apply_state_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdPhysKindApplyState>::build(),
  0
);
static constexpr ecs::ComponentDesc jolt_vehicle_request_rendinsts_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("jolt_vehicle"), ecs::ComponentTypeInfo<JoltVehicleKind>()}
};
static void jolt_vehicle_request_rendinsts_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    jolt_vehicle_request_rendinsts_es(static_cast<const UpdatePhysEvent&>(evt)
        , ECS_RO_COMP(jolt_vehicle_request_rendinsts_es_comps, "jolt_vehicle", JoltVehicleKind)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc jolt_vehicle_request_rendinsts_es_es_desc
(
  "jolt_vehicle_request_rendinsts_es",
  "prog/daNetGameLibs/phys_kind_jolt_vehicle/./joltVehicleKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, jolt_vehicle_request_rendinsts_es_all_events),
  empty_span(),
  make_span(jolt_vehicle_request_rendinsts_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,nullptr,nullptr,"after_collision_sync","after_net_phys_sync");
static constexpr ecs::ComponentDesc jolt_vehicle_follow_remote_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("jolt_vehicle"), ecs::ComponentTypeInfo<JoltVehicleKind>()},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("phys_kind_net_phys"), ecs::ComponentTypeInfo<PhysKindActor>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void jolt_vehicle_follow_remote_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    jolt_vehicle_follow_remote_es(static_cast<const UpdatePhysEvent&>(evt)
        , ECS_RO_COMP(jolt_vehicle_follow_remote_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(jolt_vehicle_follow_remote_es_comps, "phys_kind_net_phys", PhysKindActor)
    , ECS_RW_COMP(jolt_vehicle_follow_remote_es_comps, "jolt_vehicle", JoltVehicleKind)
    , ECS_RO_COMP(jolt_vehicle_follow_remote_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc jolt_vehicle_follow_remote_es_es_desc
(
  "jolt_vehicle_follow_remote_es",
  "prog/daNetGameLibs/phys_kind_jolt_vehicle/./joltVehicleKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, jolt_vehicle_follow_remote_es_all_events),
  make_span(jolt_vehicle_follow_remote_es_comps+0, 1)/*rw*/,
  make_span(jolt_vehicle_follow_remote_es_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,"gameClient",nullptr,nullptr,"after_net_phys_sync");
static constexpr ecs::ComponentDesc jolt_vehicle_sync_nodes_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("jolt_vehicle"), ecs::ComponentTypeInfo<JoltVehicleKind>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
//start of 1 ro components at [2]
  {ECS_HASH("phys_kind_net_phys"), ecs::ComponentTypeInfo<PhysKindActor>()}
};
static void jolt_vehicle_sync_nodes_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    jolt_vehicle_sync_nodes_es(static_cast<const UpdatePhysEvent&>(evt)
        , ECS_RO_COMP(jolt_vehicle_sync_nodes_es_comps, "phys_kind_net_phys", PhysKindActor)
    , ECS_RW_COMP(jolt_vehicle_sync_nodes_es_comps, "jolt_vehicle", JoltVehicleKind)
    , ECS_RW_COMP(jolt_vehicle_sync_nodes_es_comps, "animchar", AnimV20::AnimcharBaseComponent)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc jolt_vehicle_sync_nodes_es_es_desc
(
  "jolt_vehicle_sync_nodes_es",
  "prog/daNetGameLibs/phys_kind_jolt_vehicle/./joltVehicleKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, jolt_vehicle_sync_nodes_es_all_events),
  make_span(jolt_vehicle_sync_nodes_es_comps+0, 2)/*rw*/,
  make_span(jolt_vehicle_sync_nodes_es_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,nullptr,nullptr,nullptr,"after_net_phys_sync");
