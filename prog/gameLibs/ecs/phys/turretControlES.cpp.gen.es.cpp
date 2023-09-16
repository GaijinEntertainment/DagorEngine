#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t turret__owner_get_type();
static ecs::LTComponentList turret__owner_component(ECS_HASH("turret__owner"), turret__owner_get_type(), "prog/gameLibs/ecs/phys/turretControlES.cpp.inl", "update_turret_payload_es", 0);
#include "turretControlES.cpp.inl"
ECS_DEF_PULL_VAR(turretControl);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc turret_control_init_guns_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("turret_control__gunEids"), ecs::ComponentTypeInfo<ecs::EidList>()}
};
static void turret_control_init_guns_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    turret_control_init_guns_es_event_handler(evt
        , ECS_RO_COMP(turret_control_init_guns_es_event_handler_comps, "turret_control__gunEids", ecs::EidList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc turret_control_init_guns_es_event_handler_es_desc
(
  "turret_control_init_guns_es",
  "prog/gameLibs/ecs/phys/turretControlES.cpp.inl",
  ecs::EntitySystemOps(nullptr, turret_control_init_guns_es_event_handler_all_events),
  empty_span(),
  make_span(turret_control_init_guns_es_event_handler_comps+0, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"netClient");
static constexpr ecs::ComponentDesc turret_control_create_guns_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("turretsInitialComponents"), ecs::ComponentTypeInfo<ecs::Array>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("turret_control__gunEids"), ecs::ComponentTypeInfo<ecs::EidList>()},
//start of 2 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("turret_control__turretInfo"), ecs::ComponentTypeInfo<ecs::SharedComponent<ecs::Array>>()}
};
static void turret_control_create_guns_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    turret_control_create_guns_es_event_handler(evt
        , components.manager()
    , ECS_RO_COMP(turret_control_create_guns_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP(turret_control_create_guns_es_event_handler_comps, "turret_control__turretInfo", ecs::SharedComponent<ecs::Array>)
    , ECS_RW_COMP_PTR(turret_control_create_guns_es_event_handler_comps, "turretsInitialComponents", ecs::Array)
    , ECS_RW_COMP(turret_control_create_guns_es_event_handler_comps, "turret_control__gunEids", ecs::EidList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc turret_control_create_guns_es_event_handler_es_desc
(
  "turret_control_create_guns_es",
  "prog/gameLibs/ecs/phys/turretControlES.cpp.inl",
  ecs::EntitySystemOps(nullptr, turret_control_create_guns_es_event_handler_all_events),
  make_span(turret_control_create_guns_es_event_handler_comps+0, 2)/*rw*/,
  make_span(turret_control_create_guns_es_event_handler_comps+2, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
,"server");
static constexpr ecs::ComponentDesc turret_control_destroy_guns_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("turret_control__gunEids"), ecs::ComponentTypeInfo<ecs::EidList>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void turret_control_destroy_guns_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    turret_control_destroy_guns_es_event_handler(evt
        , components.manager()
    , ECS_RO_COMP(turret_control_destroy_guns_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(turret_control_destroy_guns_es_event_handler_comps, "turret_control__gunEids", ecs::EidList)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc turret_control_destroy_guns_es_event_handler_es_desc
(
  "turret_control_destroy_guns_es",
  "prog/gameLibs/ecs/phys/turretControlES.cpp.inl",
  ecs::EntitySystemOps(nullptr, turret_control_destroy_guns_es_event_handler_all_events),
  make_span(turret_control_destroy_guns_es_event_handler_comps+0, 1)/*rw*/,
  make_span(turret_control_destroy_guns_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
,"server");
static constexpr ecs::ComponentDesc update_turret_payload_es_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("turret_control__gunEids"), ecs::ComponentTypeInfo<ecs::EidList>()},
  {ECS_HASH("turret_control__hasPayload"), ecs::ComponentTypeInfo<bool>()}
};
static void update_turret_payload_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<ParallelUpdateFrameDelayed>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(update_turret_payload_es_comps, "turret_control__hasPayload", bool)) )
      continue;
    update_turret_payload_es(static_cast<const ParallelUpdateFrameDelayed&>(evt)
          , components.manager()
      , ECS_RO_COMP(update_turret_payload_es_comps, "eid", ecs::EntityId)
      , ECS_RO_COMP(update_turret_payload_es_comps, "turret_control__gunEids", ecs::EidList)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc update_turret_payload_es_es_desc
(
  "update_turret_payload_es",
  "prog/gameLibs/ecs/phys/turretControlES.cpp.inl",
  ecs::EntitySystemOps(nullptr, update_turret_payload_es_all_events),
  empty_span(),
  make_span(update_turret_payload_es_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ParallelUpdateFrameDelayed>::build(),
  0
,nullptr,nullptr,"*");
static constexpr ecs::ComponentDesc turret_gun_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("gun"), ecs::ComponentTypeInfo<ecs::auto_type>()}
};
static ecs::CompileTimeQueryDesc turret_gun_ecs_query_desc
(
  "turret_gun_ecs_query",
  empty_span(),
  make_span(turret_gun_ecs_query_comps+0, 1)/*ro*/,
  make_span(turret_gun_ecs_query_comps+1, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void turret_gun_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, turret_gun_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              components.manager()
            , ECS_RO_COMP(turret_gun_ecs_query_comps, "eid", ecs::EntityId)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc turret_payload_gun_ammo_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("gun__ammo"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("gun__isPayload"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("gun"), ecs::ComponentTypeInfo<ecs::auto_type>()}
};
static ecs::CompileTimeQueryDesc turret_payload_gun_ammo_ecs_query_desc
(
  "turret_payload_gun_ammo_ecs_query",
  empty_span(),
  make_span(turret_payload_gun_ammo_ecs_query_comps+0, 2)/*ro*/,
  make_span(turret_payload_gun_ammo_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void turret_payload_gun_ammo_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, turret_payload_gun_ammo_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          if ( !(ECS_RO_COMP(turret_payload_gun_ammo_ecs_query_comps, "gun__isPayload", bool)) )
            return;
          function(
              ECS_RO_COMP(turret_payload_gun_ammo_ecs_query_comps, "gun__ammo", int)
            );

        }
      }
  );
}
static constexpr ecs::component_t turret__owner_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
