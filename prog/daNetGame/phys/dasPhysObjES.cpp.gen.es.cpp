#include "dasPhysObjES.cpp.inl"
ECS_DEF_PULL_VAR(dasPhysObj);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc das_phys_obj_walker_phys_es_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("das_phys_obj_net_phys"), ecs::ComponentTypeInfo<DasPhysObjActor>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 ro components at [2]
  {ECS_HASH("updatable"), ecs::ComponentTypeInfo<bool>()}
};
static void das_phys_obj_walker_phys_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
  {
    if ( !(ECS_RO_COMP(das_phys_obj_walker_phys_es_comps, "updatable", bool)) )
      continue;
    das_phys_obj_walker_phys_es(static_cast<const UpdatePhysEvent&>(evt)
          , ECS_RW_COMP(das_phys_obj_walker_phys_es_comps, "das_phys_obj_net_phys", DasPhysObjActor)
      , ECS_RW_COMP(das_phys_obj_walker_phys_es_comps, "transform", TMatrix)
      );
  } while (++comp != compE);
}
static ecs::EntitySystemDesc das_phys_obj_walker_phys_es_es_desc
(
  "das_phys_obj_walker_phys_es",
  "prog/daNetGame/phys/dasPhysObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, das_phys_obj_walker_phys_es_all_events),
  make_span(das_phys_obj_walker_phys_es_comps+0, 2)/*rw*/,
  make_span(das_phys_obj_walker_phys_es_comps+2, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,nullptr,nullptr,"after_net_phys_sync","before_net_phys_sync");
static constexpr ecs::ComponentDesc das_phys_obj_phys_events_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("das_phys_obj_net_phys"), ecs::ComponentTypeInfo<DasPhysObjActor>()}
};
static void das_phys_obj_phys_events_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    das_phys_obj_phys_events_es_event_handler(evt
        , ECS_RW_COMP(das_phys_obj_phys_events_es_event_handler_comps, "das_phys_obj_net_phys", DasPhysObjActor)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc das_phys_obj_phys_events_es_event_handler_es_desc
(
  "das_phys_obj_phys_events_es",
  "prog/daNetGame/phys/dasPhysObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, das_phys_obj_phys_events_es_event_handler_all_events),
  make_span(das_phys_obj_phys_events_es_event_handler_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdTeleportEntity,
                       ecs::EventNetMessage>::build(),
  0
);
static constexpr ecs::ComponentDesc das_phys_obj_net_phys_apply_authority_state_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("das_phys_obj_net_phys"), ecs::ComponentTypeInfo<DasPhysObjActor>()},
//start of 1 no components at [1]
  {ECS_HASH("disableUpdate"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void das_phys_obj_net_phys_apply_authority_state_es_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<UpdatePhysEvent>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    das_phys_obj_net_phys_apply_authority_state_es(static_cast<const UpdatePhysEvent&>(evt)
        , ECS_RW_COMP(das_phys_obj_net_phys_apply_authority_state_es_comps, "das_phys_obj_net_phys", DasPhysObjActor)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc das_phys_obj_net_phys_apply_authority_state_es_es_desc
(
  "das_phys_obj_net_phys_apply_authority_state_es",
  "prog/daNetGame/phys/dasPhysObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, das_phys_obj_net_phys_apply_authority_state_es_all_events),
  make_span(das_phys_obj_net_phys_apply_authority_state_es_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  make_span(das_phys_obj_net_phys_apply_authority_state_es_comps+1, 1)/*no*/,
  ecs::EventSetBuilder<UpdatePhysEvent>::build(),
  0
,nullptr,nullptr,"after_net_phys_sync,das_phys_obj_walker_phys_es","before_net_phys_sync");
//static constexpr ecs::ComponentDesc register_das_phys_obj_net_phys_phys_on_appinit_es_event_handler_comps[] ={};
static void register_das_phys_obj_net_phys_phys_on_appinit_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnGameInit>());
  register_das_phys_obj_net_phys_phys_on_appinit_es_event_handler(static_cast<const EventOnGameInit&>(evt)
        );
}
static ecs::EntitySystemDesc register_das_phys_obj_net_phys_phys_on_appinit_es_event_handler_es_desc
(
  "register_das_phys_obj_net_phys_phys_on_appinit_es",
  "prog/daNetGame/phys/dasPhysObjES.cpp.inl",
  ecs::EntitySystemOps(nullptr, register_das_phys_obj_net_phys_phys_on_appinit_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnGameInit>::build(),
  0
,nullptr,nullptr,"*");
