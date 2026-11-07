// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "physKindES.cpp.inl"
ECS_DEF_PULL_VAR(physKind);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc phys_kind_apply_authority_state_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("phys_kind_net_phys"), ecs::ComponentTypeInfo<PhysKindActor>()},
//start of 3 ro components at [1]
  {ECS_HASH("net__physId"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("net_phys__disableAuthorityStateApplication"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("net_phys__smoothAuthorityApplication"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [4]
  {ECS_HASH("disableUpdate"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void phys_kind_apply_authority_state_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_kind_apply_authority_state_es(static_cast<const UpdatePhysEvent&>(evt)
        , ECS_RW_COMP(phys_kind_apply_authority_state_es_comps, "phys_kind_net_phys", PhysKindActor)
    , ECS_RO_COMP(phys_kind_apply_authority_state_es_comps, "net__physId", int)
    , ECS_RO_COMP_OR(phys_kind_apply_authority_state_es_comps, "net_phys__disableAuthorityStateApplication", bool(false))
    , ECS_RO_COMP_OR(phys_kind_apply_authority_state_es_comps, "net_phys__smoothAuthorityApplication", bool(true))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_kind_apply_authority_state_es_es_desc
(
  "phys_kind_apply_authority_state_es",
  "prog/daNetGameLibs/phys_kind/./physKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_kind_apply_authority_state_es_all_events),
  make_span(phys_kind_apply_authority_state_es_comps+0, 1)/*rw*/,
  make_span(phys_kind_apply_authority_state_es_comps+1, 3)/*ro*/,
  empty_span(),
  make_span(phys_kind_apply_authority_state_es_comps+4, 1)/*no*/,
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,nullptr,nullptr,"after_net_phys_sync,phys_kind_phys_es","before_net_phys_sync");
static constexpr ecs::ComponentDesc phys_kind_phys_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("phys_kind_net_phys"), ecs::ComponentTypeInfo<PhysKindActor>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 no components at [2]
  {ECS_HASH("disableUpdate"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void phys_kind_phys_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_kind_phys_es(static_cast<const UpdatePhysEvent&>(evt)
        , ECS_RW_COMP(phys_kind_phys_es_comps, "phys_kind_net_phys", PhysKindActor)
    , ECS_RW_COMP(phys_kind_phys_es_comps, "transform", TMatrix)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_kind_phys_es_es_desc
(
  "phys_kind_phys_es",
  "prog/daNetGameLibs/phys_kind/./physKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_kind_phys_es_all_events),
  make_span(phys_kind_phys_es_comps+0, 2)/*rw*/,
  empty_span(),
  empty_span(),
  make_span(phys_kind_phys_es_comps+2, 1)/*no*/,
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,nullptr,nullptr,"after_net_phys_sync","before_net_phys_sync");
static constexpr ecs::ComponentDesc phys_kind_phys_events_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("phys_kind_net_phys"), ecs::ComponentTypeInfo<PhysKindActor>()}
};
static void phys_kind_phys_events_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_kind_phys_events_es_event_handler(evt
        , ECS_RW_COMP(phys_kind_phys_events_es_event_handler_comps, "phys_kind_net_phys", PhysKindActor)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_kind_phys_events_es_event_handler_es_desc
(
  "phys_kind_phys_events_es",
  "prog/daNetGameLibs/phys_kind/./physKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_kind_phys_events_es_event_handler_all_events),
  make_span(phys_kind_phys_events_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdTeleportEntity,
                       ecs::EventNetMessage>::build(),
  0
);
//static constexpr ecs::ComponentDesc phys_kind_server_world_step_es_comps[] ={};
static void phys_kind_server_world_step_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  phys_kind_server_world_step_es(static_cast<const UpdatePhysEvent&>(evt)
        );
}
static ecs::EntitySystemDesc phys_kind_server_world_step_es_es_desc
(
  "phys_kind_server_world_step_es",
  "prog/daNetGameLibs/phys_kind/./physKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_kind_server_world_step_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,"server",nullptr,nullptr,"after_collision_sync");
static constexpr ecs::ComponentDesc phys_kind_net_phys_on_tickrate_changed_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("phys_kind_net_phys"), ecs::ComponentTypeInfo<PhysKindActor>()},
//start of 1 ro components at [1]
  {ECS_HASH("phys__lowFreqTickrate"), ecs::ComponentTypeInfo<bool>()}
};
static void phys_kind_net_phys_on_tickrate_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_kind_net_phys_on_tickrate_changed_es_event_handler(evt
        , ECS_RW_COMP(phys_kind_net_phys_on_tickrate_changed_es_event_handler_comps, "phys_kind_net_phys", PhysKindActor)
    , ECS_RO_COMP(phys_kind_net_phys_on_tickrate_changed_es_event_handler_comps, "phys__lowFreqTickrate", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_kind_net_phys_on_tickrate_changed_es_event_handler_es_desc
(
  "phys_kind_net_phys_on_tickrate_changed_es",
  "prog/daNetGameLibs/phys_kind/./physKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_kind_net_phys_on_tickrate_changed_es_event_handler_all_events),
  make_span(phys_kind_net_phys_on_tickrate_changed_es_event_handler_comps+0, 1)/*rw*/,
  make_span(phys_kind_net_phys_on_tickrate_changed_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"phys__lowFreqTickrate");
static constexpr ecs::ComponentDesc phys_kind_net_phys_post_update_es_comps[] =
{
//start of 9 rw components at [0]
  {ECS_HASH("net_phys__atTick"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("net_phys__lastAppliedControlsForTick"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("net_phys__timeStep"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("net_phys__currentStateVelocity"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("net_phys__currentStatePosition"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("net_phys__currentStateOrient"), ecs::ComponentTypeInfo<Point4>()},
  {ECS_HASH("net_phys__previousStateVelocity"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("net_phys__role"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("net_phys__interpK"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
//start of 1 ro components at [9]
  {ECS_HASH("phys_kind_net_phys"), ecs::ComponentTypeInfo<PhysKindActor>()}
};
static void phys_kind_net_phys_post_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_kind_net_phys_post_update_es(static_cast<const UpdatePhysEvent&>(evt)
        , ECS_RO_COMP(phys_kind_net_phys_post_update_es_comps, "phys_kind_net_phys", PhysKindActor)
    , ECS_RW_COMP(phys_kind_net_phys_post_update_es_comps, "net_phys__atTick", int)
    , ECS_RW_COMP(phys_kind_net_phys_post_update_es_comps, "net_phys__lastAppliedControlsForTick", int)
    , ECS_RW_COMP(phys_kind_net_phys_post_update_es_comps, "net_phys__timeStep", float)
    , ECS_RW_COMP(phys_kind_net_phys_post_update_es_comps, "net_phys__currentStateVelocity", Point3)
    , ECS_RW_COMP(phys_kind_net_phys_post_update_es_comps, "net_phys__currentStatePosition", Point3)
    , ECS_RW_COMP(phys_kind_net_phys_post_update_es_comps, "net_phys__currentStateOrient", Point4)
    , ECS_RW_COMP(phys_kind_net_phys_post_update_es_comps, "net_phys__previousStateVelocity", Point3)
    , ECS_RW_COMP(phys_kind_net_phys_post_update_es_comps, "net_phys__role", int)
    , ECS_RW_COMP_PTR(phys_kind_net_phys_post_update_es_comps, "net_phys__interpK", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_kind_net_phys_post_update_es_es_desc
(
  "phys_kind_net_phys_post_update_es",
  "prog/daNetGameLibs/phys_kind/./physKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_kind_net_phys_post_update_es_all_events),
  make_span(phys_kind_net_phys_post_update_es_comps+0, 9)/*rw*/,
  make_span(phys_kind_net_phys_post_update_es_comps+9, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,nullptr,nullptr,"after_collision_sync","after_net_phys_sync");
//static constexpr ecs::ComponentDesc register_phys_kind_net_phys_phys_on_appinit_es_event_handler_comps[] ={};
static void register_phys_kind_net_phys_phys_on_appinit_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnGameInit>());
  register_phys_kind_net_phys_phys_on_appinit_es_event_handler(static_cast<const EventOnGameInit&>(evt)
        );
}
static ecs::EntitySystemDesc register_phys_kind_net_phys_phys_on_appinit_es_event_handler_es_desc
(
  "register_phys_kind_net_phys_phys_on_appinit_es",
  "prog/daNetGameLibs/phys_kind/./physKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_phys_kind_net_phys_phys_on_appinit_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnGameInit>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc gather_phys_kind_net_phys_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("phys_kind_net_phys"), ecs::ComponentTypeInfo<PhysKindActor>()},
  {ECS_HASH("physAlwaysSendSnapshot"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
//start of 5 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("replication"), ecs::ComponentTypeInfo<net::Object>()},
  {ECS_HASH("team"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("netLodZones"), ecs::ComponentTypeInfo<Point4>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("phys_kind__snapshotHz"), ecs::ComponentTypeInfo<Point4>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [7]
  {ECS_HASH("phys__disableSnapshots"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void gather_phys_kind_net_phys_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<GatherNetPhysRecordsToSend>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gather_phys_kind_net_phys_es_event_handler(static_cast<const GatherNetPhysRecordsToSend&>(evt)
        , ECS_RO_COMP(gather_phys_kind_net_phys_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(gather_phys_kind_net_phys_es_event_handler_comps, "phys_kind_net_phys", PhysKindActor)
    , ECS_RO_COMP(gather_phys_kind_net_phys_es_event_handler_comps, "replication", net::Object)
    , ECS_RO_COMP_PTR(gather_phys_kind_net_phys_es_event_handler_comps, "team", int)
    , ECS_RO_COMP_PTR(gather_phys_kind_net_phys_es_event_handler_comps, "netLodZones", Point4)
    , ECS_RO_COMP_PTR(gather_phys_kind_net_phys_es_event_handler_comps, "phys_kind__snapshotHz", Point4)
    , ECS_RW_COMP_PTR(gather_phys_kind_net_phys_es_event_handler_comps, "physAlwaysSendSnapshot", ecs::Tag)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gather_phys_kind_net_phys_es_event_handler_es_desc
(
  "gather_phys_kind_net_phys_es",
  "prog/daNetGameLibs/phys_kind/./physKindES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gather_phys_kind_net_phys_es_event_handler_all_events),
  make_span(gather_phys_kind_net_phys_es_event_handler_comps+0, 2)/*rw*/,
  make_span(gather_phys_kind_net_phys_es_event_handler_comps+2, 5)/*ro*/,
  empty_span(),
  make_span(gather_phys_kind_net_phys_es_event_handler_comps+7, 1)/*no*/,
  ecs::EventSetBuilder<GatherNetPhysRecordsToSend>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc phys_kind_external_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("phys_kind_net_phys"), ecs::ComponentTypeInfo<PhysKindActor>()}
};
static ecs::CompileTimeQueryDesc phys_kind_external_ecs_query_desc
(
  "phys_kind_external_ecs_query",
  empty_span(),
  make_span(phys_kind_external_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline ecs::QueryCbResult phys_kind_external_ecs_query(Callable function)
{
  return perform_query(g_entity_mgr, phys_kind_external_ecs_query_desc.getHandle(),
    ecs::stoppable_query_cb_t([&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(phys_kind_external_ecs_query_comps, "phys_kind_net_phys", PhysKindActor)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    })
  );
}
