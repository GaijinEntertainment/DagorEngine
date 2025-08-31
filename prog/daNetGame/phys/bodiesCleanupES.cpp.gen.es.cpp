#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t bodies_cleanup_get_type();
static ecs::LTComponentList bodies_cleanup_component(ECS_HASH("bodies_cleanup"), bodies_cleanup_get_type(), "prog/daNetGame/phys/bodiesCleanupES.cpp.inl", "update_body_cleanup_ttl_es", 0);
static constexpr ecs::component_t canRemoveWhilePossessed_get_type();
static ecs::LTComponentList canRemoveWhilePossessed_component(ECS_HASH("canRemoveWhilePossessed"), canRemoveWhilePossessed_get_type(), "prog/daNetGame/phys/bodiesCleanupES.cpp.inl", "bodies_cleanup_es", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "bodiesCleanupES.cpp.inl"
ECS_DEF_PULL_VAR(bodiesCleanup);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc bodies_cleanup_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("bodies_cleanup"), ecs::ComponentTypeInfo<BodiesCleanup>()}
};
static void bodies_cleanup_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    bodies_cleanup_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(bodies_cleanup_es_comps, "bodies_cleanup", BodiesCleanup)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bodies_cleanup_es_es_desc
(
  "bodies_cleanup_es",
  "prog/daNetGame/phys/bodiesCleanupES.cpp.inl",
  ecs::EntitySystemOps(bodies_cleanup_es_all),
  make_span(bodies_cleanup_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc bodies_cmd_cleanup_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("replication"), ecs::ComponentTypeInfo<net::Object>(), ecs::CDF_OPTIONAL},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void bodies_cmd_cleanup_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<CmdBodyCleanup>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    bodies_cmd_cleanup_es_event_handler(static_cast<const CmdBodyCleanup&>(evt)
        , ECS_RO_COMP(bodies_cmd_cleanup_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP_PTR(bodies_cmd_cleanup_es_event_handler_comps, "replication", net::Object)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bodies_cmd_cleanup_es_event_handler_es_desc
(
  "bodies_cmd_cleanup_es",
  "prog/daNetGame/phys/bodiesCleanupES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bodies_cmd_cleanup_es_event_handler_all_events),
  make_span(bodies_cmd_cleanup_es_event_handler_comps+0, 1)/*rw*/,
  make_span(bodies_cmd_cleanup_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdBodyCleanup>::build(),
  0
);
static constexpr ecs::ComponentDesc update_body_cleanup_ttl_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void update_body_cleanup_ttl_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<CmdBodyCleanupUpdateTtl>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    update_body_cleanup_ttl_es(static_cast<const CmdBodyCleanupUpdateTtl&>(evt)
        , ECS_RO_COMP(update_body_cleanup_ttl_es_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc update_body_cleanup_ttl_es_es_desc
(
  "update_body_cleanup_ttl_es",
  "prog/daNetGame/phys/bodiesCleanupES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_body_cleanup_ttl_es_all_events),
  empty_span(),
  make_span(update_body_cleanup_ttl_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdBodyCleanupUpdateTtl>::build(),
  0
);
static constexpr ecs::ComponentDesc bodies_resurrected_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("bodies_cleanup"), ecs::ComponentTypeInfo<BodiesCleanup>()}
};
static void bodies_resurrected_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventAnyEntityResurrected>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    bodies_resurrected_es_event_handler(static_cast<const EventAnyEntityResurrected&>(evt)
        , ECS_RW_COMP(bodies_resurrected_es_event_handler_comps, "bodies_cleanup", BodiesCleanup)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc bodies_resurrected_es_event_handler_es_desc
(
  "bodies_resurrected_es",
  "prog/daNetGame/phys/bodiesCleanupES.cpp.inl",
  ecs::EntitySystemOps(nullptr, bodies_resurrected_es_event_handler_all_events),
  make_span(bodies_resurrected_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventAnyEntityResurrected>::build(),
  0
);
static constexpr ecs::component_t bodies_cleanup_get_type(){return ecs::ComponentTypeInfo<BodiesCleanup>::type; }
static constexpr ecs::component_t canRemoveWhilePossessed_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
