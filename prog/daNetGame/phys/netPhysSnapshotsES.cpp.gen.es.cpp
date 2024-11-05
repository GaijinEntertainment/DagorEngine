#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t isInVehicle_get_type();
static ecs::LTComponentList isInVehicle_component(ECS_HASH("isInVehicle"), isInVehicle_get_type(), "prog/daNetGame/phys/netPhysSnapshotsES.cpp.inl", "player_snapshots_ecs_query", 0);
static constexpr ecs::component_t isTpsView_get_type();
static ecs::LTComponentList isTpsView_component(ECS_HASH("isTpsView"), isTpsView_get_type(), "prog/daNetGame/phys/netPhysSnapshotsES.cpp.inl", "player_snapshots_ecs_query", 0);
#include "netPhysSnapshotsES.cpp.inl"
ECS_DEF_PULL_VAR(netPhysSnapshots);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc net_phys_receive_snapshots_es_event_handler_comps[] =
{
//start of 1 rq components at [0]
  {ECS_HASH("msg_sink"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void net_phys_receive_snapshots_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ecs::EventNetMessage>());
  net_phys_receive_snapshots_es_event_handler(static_cast<const ecs::EventNetMessage&>(evt)
        );
}
static ecs::EntitySystemDesc net_phys_receive_snapshots_es_event_handler_es_desc
(
  "net_phys_receive_snapshots_es",
  "prog/daNetGame/phys/netPhysSnapshotsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, net_phys_receive_snapshots_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  make_span(net_phys_receive_snapshots_es_event_handler_comps+0, 1)/*rq*/,
  empty_span(),
  ecs::EventSetBuilder<ecs::EventNetMessage>::build(),
  0
,"netClient");
static constexpr ecs::ComponentDesc player_snapshots_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("player"), ecs::ComponentTypeInfo<game::Player>()},
  {ECS_HASH("clientNetFlags"), ecs::ComponentTypeInfo<int>()},
//start of 3 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("connid"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("team"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [5]
  {ECS_HASH("playerIsBot"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc player_snapshots_ecs_query_desc
(
  "player_snapshots_ecs_query",
  make_span(player_snapshots_ecs_query_comps+0, 2)/*rw*/,
  make_span(player_snapshots_ecs_query_comps+2, 3)/*ro*/,
  empty_span(),
  make_span(player_snapshots_ecs_query_comps+5, 1)/*no*/);
template<typename Callable>
inline void player_snapshots_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, player_snapshots_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(player_snapshots_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(player_snapshots_ecs_query_comps, "player", game::Player)
            , ECS_RO_COMP(player_snapshots_ecs_query_comps, "connid", int)
            , ECS_RW_COMP(player_snapshots_ecs_query_comps, "clientNetFlags", int)
            , ECS_RO_COMP_OR(player_snapshots_ecs_query_comps, "team", int(TEAM_UNASSIGNED))
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::component_t isInVehicle_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
static constexpr ecs::component_t isTpsView_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
