#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t collision_physMatId_get_type();
static ecs::LTComponentList collision_physMatId_component(ECS_HASH("collision_physMatId"), collision_physMatId_get_type(), "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl", "", 0);
static constexpr ecs::component_t ignoreObjs__eids_get_type();
static ecs::LTComponentList ignoreObjs__eids_component(ECS_HASH("ignoreObjs__eids"), ignoreObjs__eids_get_type(), "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl", "", 0);
static constexpr ecs::component_t phys__disableSnapshots_get_type();
static ecs::LTComponentList phys__disableSnapshots_component(ECS_HASH("phys__disableSnapshots"), phys__disableSnapshots_get_type(), "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl", "", 0);
static constexpr ecs::component_t turret_state_remote_get_type();
static ecs::LTComponentList turret_state_remote_component(ECS_HASH("turret_state_remote"), turret_state_remote_get_type(), "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl", "", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "physObjES.cpp.inl"
ECS_DEF_PULL_VAR(physObj);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc underground_physobj_teleporter_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("underground_teleporter__timeToCheck"), ecs::ComponentTypeInfo<float>()},
//start of 4 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("phys_obj_net_phys"), ecs::ComponentTypeInfo<PhysObjActor>()},
  {ECS_HASH("underground_teleporter__timeBetweenChecks"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("underground_teleporter__heightOffset"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void underground_physobj_teleporter_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    underground_physobj_teleporter_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RO_COMP(underground_physobj_teleporter_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(underground_physobj_teleporter_es_comps, "phys_obj_net_phys", PhysObjActor)
    , ECS_RW_COMP(underground_physobj_teleporter_es_comps, "underground_teleporter__timeToCheck", float)
    , ECS_RO_COMP_OR(underground_physobj_teleporter_es_comps, "underground_teleporter__timeBetweenChecks", float(5.f))
    , ECS_RO_COMP_OR(underground_physobj_teleporter_es_comps, "underground_teleporter__heightOffset", float(0.7f))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc underground_physobj_teleporter_es_es_desc
(
  "underground_physobj_teleporter_es",
  "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl",
  ecs::EntitySystemOps(underground_physobj_teleporter_es_all),
  make_span(underground_physobj_teleporter_es_comps+0, 1)/*rw*/,
  make_span(underground_physobj_teleporter_es_comps+1, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"server",nullptr,"*");
static constexpr ecs::ComponentDesc phys_obj_apply_snapshot_cache_on_creation_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("phys_obj_net_phys"), ecs::ComponentTypeInfo<PhysObjActor>()},
//start of 1 ro components at [1]
  {ECS_HASH("net__physId"), ecs::ComponentTypeInfo<int>()}
};
static void phys_obj_apply_snapshot_cache_on_creation_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_obj_apply_snapshot_cache_on_creation_es_event_handler(evt
        , ECS_RW_COMP(phys_obj_apply_snapshot_cache_on_creation_es_event_handler_comps, "phys_obj_net_phys", PhysObjActor)
    , ECS_RO_COMP(phys_obj_apply_snapshot_cache_on_creation_es_event_handler_comps, "net__physId", int)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_obj_apply_snapshot_cache_on_creation_es_event_handler_es_desc
(
  "phys_obj_apply_snapshot_cache_on_creation_es",
  "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_obj_apply_snapshot_cache_on_creation_es_event_handler_all_events),
  make_span(phys_obj_apply_snapshot_cache_on_creation_es_event_handler_comps+0, 1)/*rw*/,
  make_span(phys_obj_apply_snapshot_cache_on_creation_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"netClient");
static constexpr ecs::ComponentDesc phys_obj_phys_es_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("phys_obj_net_phys"), ecs::ComponentTypeInfo<PhysObjActor>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("net_phys__prevTick"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
//start of 1 ro components at [3]
  {ECS_HASH("phys__broadcastAAS"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [4]
  {ECS_HASH("disableUpdate"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void phys_obj_phys_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_obj_phys_es(static_cast<const UpdatePhysEvent&>(evt)
        , ECS_RW_COMP(phys_obj_phys_es_comps, "phys_obj_net_phys", PhysObjActor)
    , ECS_RW_COMP(phys_obj_phys_es_comps, "transform", TMatrix)
    , ECS_RW_COMP_PTR(phys_obj_phys_es_comps, "net_phys__prevTick", int)
    , ECS_RO_COMP_PTR(phys_obj_phys_es_comps, "phys__broadcastAAS", ecs::Tag)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_obj_phys_es_es_desc
(
  "phys_obj_phys_es",
  "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_obj_phys_es_all_events),
  make_span(phys_obj_phys_es_comps+0, 3)/*rw*/,
  make_span(phys_obj_phys_es_comps+3, 1)/*ro*/,
  empty_span(),
  make_span(phys_obj_phys_es_comps+4, 1)/*no*/,
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,nullptr,nullptr,"after_net_phys_sync","before_net_phys_sync");
static constexpr ecs::ComponentDesc phys_obj_phys_events_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("phys_obj_net_phys"), ecs::ComponentTypeInfo<PhysObjActor>()}
};
static void phys_obj_phys_events_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_obj_phys_events_es_event_handler(evt
        , ECS_RW_COMP(phys_obj_phys_events_es_event_handler_comps, "phys_obj_net_phys", PhysObjActor)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_obj_phys_events_es_event_handler_es_desc
(
  "phys_obj_phys_events_es",
  "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_obj_phys_events_es_event_handler_all_events),
  make_span(phys_obj_phys_events_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdTeleportEntity,
                       ecs::EventNetMessage>::build(),
  0
);
static constexpr ecs::ComponentDesc phys_obj_net_phys_on_tickrate_changed_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("phys_obj_net_phys"), ecs::ComponentTypeInfo<PhysObjActor>()},
//start of 1 ro components at [1]
  {ECS_HASH("phys__lowFreqTickrate"), ecs::ComponentTypeInfo<bool>()}
};
static void phys_obj_net_phys_on_tickrate_changed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_obj_net_phys_on_tickrate_changed_es_event_handler(evt
        , ECS_RW_COMP(phys_obj_net_phys_on_tickrate_changed_es_event_handler_comps, "phys_obj_net_phys", PhysObjActor)
    , ECS_RO_COMP(phys_obj_net_phys_on_tickrate_changed_es_event_handler_comps, "phys__lowFreqTickrate", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_obj_net_phys_on_tickrate_changed_es_event_handler_es_desc
(
  "phys_obj_net_phys_on_tickrate_changed_es",
  "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_obj_net_phys_on_tickrate_changed_es_event_handler_all_events),
  make_span(phys_obj_net_phys_on_tickrate_changed_es_event_handler_comps+0, 1)/*rw*/,
  make_span(phys_obj_net_phys_on_tickrate_changed_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"phys__lowFreqTickrate");
static constexpr ecs::ComponentDesc phys_obj_net_phys_collision_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("phys_obj_net_phys"), ecs::ComponentTypeInfo<PhysObjActor>()},
//start of 5 ro components at [1]
  {ECS_HASH("grid_obj"), ecs::ComponentTypeInfo<GridObjComponent>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("pair_collision__gridHolders"), ecs::ComponentTypeInfo<GridHolders>()},
  {ECS_HASH("pair_collision__tag"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("havePairCollision"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static void phys_obj_net_phys_collision_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP_OR(phys_obj_net_phys_collision_es_comps, "havePairCollision", bool( true))) )
      continue;
    phys_obj_net_phys_collision_es(static_cast<const UpdatePhysEvent&>(evt)
          , ECS_RW_COMP(phys_obj_net_phys_collision_es_comps, "phys_obj_net_phys", PhysObjActor)
      , ECS_RO_COMP_PTR(phys_obj_net_phys_collision_es_comps, "grid_obj", GridObjComponent)
      , ECS_RO_COMP(phys_obj_net_phys_collision_es_comps, "collres", CollisionResource)
      , ECS_RO_COMP(phys_obj_net_phys_collision_es_comps, "pair_collision__gridHolders", GridHolders)
      , ECS_RO_COMP_PTR(phys_obj_net_phys_collision_es_comps, "pair_collision__tag", ecs::string)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc phys_obj_net_phys_collision_es_es_desc
(
  "phys_obj_net_phys_collision_es",
  "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_obj_net_phys_collision_es_all_events),
  make_span(phys_obj_net_phys_collision_es_comps+0, 1)/*rw*/,
  make_span(phys_obj_net_phys_collision_es_comps+1, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,nullptr,nullptr,"after_collision_sync","after_net_phys_sync");
static constexpr ecs::ComponentDesc phys_obj_net_phys_apply_authority_state_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("phys_obj_net_phys"), ecs::ComponentTypeInfo<PhysObjActor>()},
//start of 2 ro components at [1]
  {ECS_HASH("net_phys__disableAuthorityStateApplication"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("net_phys__smoothAuthorityApplication"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [3]
  {ECS_HASH("disableUpdate"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void phys_obj_net_phys_apply_authority_state_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_obj_net_phys_apply_authority_state_es(static_cast<const UpdatePhysEvent&>(evt)
        , ECS_RW_COMP(phys_obj_net_phys_apply_authority_state_es_comps, "phys_obj_net_phys", PhysObjActor)
    , ECS_RO_COMP_PTR(phys_obj_net_phys_apply_authority_state_es_comps, "net_phys__disableAuthorityStateApplication", bool)
    , ECS_RO_COMP_PTR(phys_obj_net_phys_apply_authority_state_es_comps, "net_phys__smoothAuthorityApplication", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_obj_net_phys_apply_authority_state_es_es_desc
(
  "phys_obj_net_phys_apply_authority_state_es",
  "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_obj_net_phys_apply_authority_state_es_all_events),
  make_span(phys_obj_net_phys_apply_authority_state_es_comps+0, 1)/*rw*/,
  make_span(phys_obj_net_phys_apply_authority_state_es_comps+1, 2)/*ro*/,
  empty_span(),
  make_span(phys_obj_net_phys_apply_authority_state_es_comps+3, 1)/*no*/,
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,nullptr,nullptr,"after_net_phys_sync,phys_obj_phys_es","before_net_phys_sync");
static constexpr ecs::ComponentDesc phys_obj_net_phys_post_update_es_comps[] =
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
  {ECS_HASH("phys_obj_net_phys"), ecs::ComponentTypeInfo<PhysObjActor>()}
};
static void phys_obj_net_phys_post_update_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    phys_obj_net_phys_post_update_es(static_cast<const UpdatePhysEvent&>(evt)
        , ECS_RO_COMP(phys_obj_net_phys_post_update_es_comps, "phys_obj_net_phys", PhysObjActor)
    , ECS_RW_COMP(phys_obj_net_phys_post_update_es_comps, "net_phys__atTick", int)
    , ECS_RW_COMP(phys_obj_net_phys_post_update_es_comps, "net_phys__lastAppliedControlsForTick", int)
    , ECS_RW_COMP(phys_obj_net_phys_post_update_es_comps, "net_phys__timeStep", float)
    , ECS_RW_COMP(phys_obj_net_phys_post_update_es_comps, "net_phys__currentStateVelocity", Point3)
    , ECS_RW_COMP(phys_obj_net_phys_post_update_es_comps, "net_phys__currentStatePosition", Point3)
    , ECS_RW_COMP(phys_obj_net_phys_post_update_es_comps, "net_phys__currentStateOrient", Point4)
    , ECS_RW_COMP(phys_obj_net_phys_post_update_es_comps, "net_phys__previousStateVelocity", Point3)
    , ECS_RW_COMP(phys_obj_net_phys_post_update_es_comps, "net_phys__role", int)
    , ECS_RW_COMP_PTR(phys_obj_net_phys_post_update_es_comps, "net_phys__interpK", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc phys_obj_net_phys_post_update_es_es_desc
(
  "phys_obj_net_phys_post_update_es",
  "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, phys_obj_net_phys_post_update_es_all_events),
  make_span(phys_obj_net_phys_post_update_es_comps+0, 9)/*rw*/,
  make_span(phys_obj_net_phys_post_update_es_comps+9, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,nullptr,nullptr,"after_collision_sync","phys_obj_net_phys_collision_es");
//static constexpr ecs::ComponentDesc register_phys_obj_net_phys_phys_on_appinit_es_event_handler_comps[] ={};
static void register_phys_obj_net_phys_phys_on_appinit_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnGameInit>());
  register_phys_obj_net_phys_phys_on_appinit_es_event_handler(static_cast<const EventOnGameInit&>(evt)
        );
}
static ecs::EntitySystemDesc register_phys_obj_net_phys_phys_on_appinit_es_event_handler_es_desc
(
  "register_phys_obj_net_phys_phys_on_appinit_es",
  "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_phys_obj_net_phys_phys_on_appinit_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnGameInit>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc gather_phys_obj_net_phys_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("phys_obj_net_phys"), ecs::ComponentTypeInfo<PhysObjActor>()},
  {ECS_HASH("physAlwaysSendSnapshot"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
//start of 5 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("replication"), ecs::ComponentTypeInfo<net::Object>()},
  {ECS_HASH("team"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("netLodZones"), ecs::ComponentTypeInfo<Point4>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("turret_control__gunEids"), ecs::ComponentTypeInfo<ecs::EidList>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [7]
  {ECS_HASH("phys__disableSnapshots"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void gather_phys_obj_net_phys_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<GatherNetPhysRecordsToSend>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    gather_phys_obj_net_phys_es_event_handler(static_cast<const GatherNetPhysRecordsToSend&>(evt)
        , ECS_RO_COMP(gather_phys_obj_net_phys_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(gather_phys_obj_net_phys_es_event_handler_comps, "phys_obj_net_phys", PhysObjActor)
    , ECS_RO_COMP(gather_phys_obj_net_phys_es_event_handler_comps, "replication", net::Object)
    , ECS_RO_COMP_PTR(gather_phys_obj_net_phys_es_event_handler_comps, "team", int)
    , ECS_RO_COMP_PTR(gather_phys_obj_net_phys_es_event_handler_comps, "netLodZones", Point4)
    , ECS_RO_COMP_PTR(gather_phys_obj_net_phys_es_event_handler_comps, "turret_control__gunEids", ecs::EidList)
    , ECS_RW_COMP_PTR(gather_phys_obj_net_phys_es_event_handler_comps, "physAlwaysSendSnapshot", ecs::Tag)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc gather_phys_obj_net_phys_es_event_handler_es_desc
(
  "gather_phys_obj_net_phys_es",
  "prog/daNetGameLibs/package_physobj/./physObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, gather_phys_obj_net_phys_es_event_handler_all_events),
  make_span(gather_phys_obj_net_phys_es_event_handler_comps+0, 2)/*rw*/,
  make_span(gather_phys_obj_net_phys_es_event_handler_comps+2, 5)/*ro*/,
  empty_span(),
  make_span(gather_phys_obj_net_phys_es_event_handler_comps+7, 1)/*no*/,
  ecs::EventSetBuilder<GatherNetPhysRecordsToSend>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::component_t collision_physMatId_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t ignoreObjs__eids_get_type(){return ecs::ComponentTypeInfo<ecs::EidList>::type; }
static constexpr ecs::component_t phys__disableSnapshots_get_type(){return ecs::ComponentTypeInfo<ecs::Tag>::type; }
static constexpr ecs::component_t turret_state_remote_get_type(){return ecs::ComponentTypeInfo<TurretState>::type; }
