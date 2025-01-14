#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t base_net_phys_ptr_get_type();
static ecs::LTComponentList base_net_phys_ptr_component(ECS_HASH("base_net_phys_ptr"), base_net_phys_ptr_get_type(), "prog/daNetGame/phys/netPhysES.cpp.inl", "", 0);
static constexpr ecs::component_t isInVehicle_get_type();
static ecs::LTComponentList isInVehicle_component(ECS_HASH("isInVehicle"), isInVehicle_get_type(), "prog/daNetGame/phys/netPhysES.cpp.inl", "", 0);
static constexpr ecs::component_t net__controlsTickDelta_get_type();
static ecs::LTComponentList net__controlsTickDelta_component(ECS_HASH("net__controlsTickDelta"), net__controlsTickDelta_get_type(), "prog/daNetGame/phys/netPhysES.cpp.inl", "", 0);
#include "netPhysES.cpp.inl"
ECS_DEF_PULL_VAR(netPhys);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc net_phys_update_es_comps[] ={};
static void net_phys_update_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    net_phys_update_es(*info.cast<ecs::UpdateStageInfoAct>());
}
static ecs::EntitySystemDesc net_phys_update_es_es_desc
(
  "net_phys_update_es",
  "prog/daNetGame/phys/netPhysES.cpp.inl",
  ecs::EntitySystemOps(net_phys_update_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
);
//static constexpr ecs::ComponentDesc start_async_phys_sim_es_comps[] ={};
static void start_async_phys_sim_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  start_async_phys_sim_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
        );
}
static ecs::EntitySystemDesc start_async_phys_sim_es_es_desc
(
  "start_async_phys_sim_es",
  "prog/daNetGame/phys/netPhysES.cpp.inl",
  ecs::EntitySystemOps(nullptr, start_async_phys_sim_es_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,"gameClient");
static constexpr ecs::ComponentDesc player_controls_sequence_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("player"), ecs::ComponentTypeInfo<game::Player>()}
};
static void player_controls_sequence_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ecs::EventNetMessage>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    player_controls_sequence_es_event_handler(static_cast<const ecs::EventNetMessage&>(evt)
        , ECS_RW_COMP(player_controls_sequence_es_event_handler_comps, "player", game::Player)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc player_controls_sequence_es_event_handler_es_desc
(
  "player_controls_sequence_es",
  "prog/daNetGame/phys/netPhysES.cpp.inl",
  ecs::EntitySystemOps(nullptr, player_controls_sequence_es_event_handler_all_events),
  make_span(player_controls_sequence_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventNetMessage>::build(),
  0
);
static constexpr ecs::ComponentDesc pair_collision_init_grid_holders_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("pair_collision__gridHolders"), ecs::ComponentTypeInfo<GridHolders>()},
//start of 1 ro components at [1]
  {ECS_HASH("pair_collision__gridNames"), ecs::ComponentTypeInfo<ecs::StringList>()}
};
static void pair_collision_init_grid_holders_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    pair_collision_init_grid_holders_es_event_handler(evt
        , ECS_RW_COMP(pair_collision_init_grid_holders_es_event_handler_comps, "pair_collision__gridHolders", GridHolders)
    , ECS_RO_COMP(pair_collision_init_grid_holders_es_event_handler_comps, "pair_collision__gridNames", ecs::StringList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc pair_collision_init_grid_holders_es_event_handler_es_desc
(
  "pair_collision_init_grid_holders_es",
  "prog/daNetGame/phys/netPhysES.cpp.inl",
  ecs::EntitySystemOps(nullptr, pair_collision_init_grid_holders_es_event_handler_all_events),
  make_span(pair_collision_init_grid_holders_es_event_handler_comps+0, 1)/*rw*/,
  make_span(pair_collision_init_grid_holders_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc deny_collision_by_ignore_list_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("collidable_ignore_list"), ecs::ComponentTypeInfo<ecs::EidList>()}
};
static void deny_collision_by_ignore_list_es_all_events(ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<QueryPhysActorsNotCollidable>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    deny_collision_by_ignore_list_es(static_cast<QueryPhysActorsNotCollidable&>(evt)
        , ECS_RO_COMP(deny_collision_by_ignore_list_es_comps, "collidable_ignore_list", ecs::EidList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc deny_collision_by_ignore_list_es_es_desc
(
  "deny_collision_by_ignore_list_es",
  "prog/daNetGame/phys/netPhysES.cpp.inl",
  ecs::EntitySystemOps(nullptr, deny_collision_by_ignore_list_es_all_events),
  empty_span(),
  make_span(deny_collision_by_ignore_list_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<QueryPhysActorsNotCollidable>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc deny_collision_for_disabled_paircoll_es_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("havePairCollision"), ecs::ComponentTypeInfo<bool>()}
};
static void deny_collision_for_disabled_paircoll_es_all_events(ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<QueryPhysActorsNotCollidable>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    deny_collision_for_disabled_paircoll_es(static_cast<QueryPhysActorsNotCollidable&>(evt)
        , ECS_RO_COMP(deny_collision_for_disabled_paircoll_es_comps, "havePairCollision", bool)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc deny_collision_for_disabled_paircoll_es_es_desc
(
  "deny_collision_for_disabled_paircoll_es",
  "prog/daNetGame/phys/netPhysES.cpp.inl",
  ecs::EntitySystemOps(nullptr, deny_collision_for_disabled_paircoll_es_all_events),
  empty_span(),
  make_span(deny_collision_for_disabled_paircoll_es_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<QueryPhysActorsNotCollidable>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc get_current_local_delay_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("time_speed__accumulatedDelay"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc get_current_local_delay_ecs_query_desc
(
  "get_current_local_delay_ecs_query",
  empty_span(),
  make_span(get_current_local_delay_ecs_query_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void get_current_local_delay_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, get_current_local_delay_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(get_current_local_delay_ecs_query_comps, "time_speed__accumulatedDelay", float)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc collision_obj_eid_ecs_query_comps[] =
{
//start of 11 ro components at [0]
  {ECS_HASH("pair_collision__tag"), ecs::ComponentTypeInfo<ecs::string>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("human"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("airplane"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("phys__kinematicBody"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("phys__hasCustomMoveLogic"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("humanAdditionalCollisionChecks"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("nphysPairCollision"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("havePairCollision"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("phys__maxMassRatioForPushOnCollision"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("net_phys__collisionMaterialId"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("net_phys__ignoreMaterialId"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc collision_obj_eid_ecs_query_desc
(
  "collision_obj_eid_ecs_query",
  empty_span(),
  make_span(collision_obj_eid_ecs_query_comps+0, 11)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void collision_obj_eid_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, collision_obj_eid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP_PTR(collision_obj_eid_ecs_query_comps, "pair_collision__tag", ecs::string)
            , ECS_RO_COMP_PTR(collision_obj_eid_ecs_query_comps, "human", ecs::Tag)
            , ECS_RO_COMP_PTR(collision_obj_eid_ecs_query_comps, "airplane", ecs::Tag)
            , ECS_RO_COMP_PTR(collision_obj_eid_ecs_query_comps, "phys__kinematicBody", ecs::Tag)
            , ECS_RO_COMP_PTR(collision_obj_eid_ecs_query_comps, "phys__hasCustomMoveLogic", ecs::Tag)
            , ECS_RO_COMP_PTR(collision_obj_eid_ecs_query_comps, "humanAdditionalCollisionChecks", ecs::Tag)
            , ECS_RO_COMP_PTR(collision_obj_eid_ecs_query_comps, "nphysPairCollision", ecs::Tag)
            , ECS_RO_COMP_OR(collision_obj_eid_ecs_query_comps, "havePairCollision", bool(true))
            , ECS_RO_COMP_OR(collision_obj_eid_ecs_query_comps, "phys__maxMassRatioForPushOnCollision", float(-1.f))
            , ECS_RO_COMP_OR(collision_obj_eid_ecs_query_comps, "net_phys__collisionMaterialId", int((int)PHYSMAT_INVALID))
            , ECS_RO_COMP_OR(collision_obj_eid_ecs_query_comps, "net_phys__ignoreMaterialId", int((int)PHYSMAT_INVALID))
            );

        }
    }
  );
}
static constexpr ecs::component_t base_net_phys_ptr_get_type(){return ecs::ComponentTypeInfo<BasePhysActor *>::type; }
static constexpr ecs::component_t isInVehicle_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
static constexpr ecs::component_t net__controlsTickDelta_get_type(){return ecs::ComponentTypeInfo<int>::type; }
