#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t riOffender_get_type();
static ecs::LTComponentList riOffender_component(ECS_HASH("riOffender"), riOffender_get_type(), "prog/daNetGame/game/riDestrES.cpp.inl", "", 0);
static constexpr ecs::component_t ri_extra__destroyed_get_type();
static ecs::LTComponentList ri_extra__destroyed_component(ECS_HASH("ri_extra__destroyed"), ri_extra__destroyed_get_type(), "prog/daNetGame/game/riDestrES.cpp.inl", "", 0);
static constexpr ecs::component_t ri_extra__isBeingReplaced_get_type();
static ecs::LTComponentList ri_extra__isBeingReplaced_component(ECS_HASH("ri_extra__isBeingReplaced"), ri_extra__isBeingReplaced_get_type(), "prog/daNetGame/game/riDestrES.cpp.inl", "", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGame/game/riDestrES.cpp.inl", "players_ecs_query", 0);
// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "riDestrES.cpp.inl"
ECS_DEF_PULL_VAR(riDestr);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc rendinst_destr_es_event_handler_comps[] ={};
static void rendinst_destr_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventLevelLoaded>());
  rendinst_destr_es_event_handler(static_cast<const EventLevelLoaded&>(evt)
        );
}
static ecs::EntitySystemDesc rendinst_destr_es_event_handler_es_desc
(
  "rendinst_destr_es",
  "prog/daNetGame/game/riDestrES.cpp.inl",
  ecs::EntitySystemOps(nullptr, rendinst_destr_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventLevelLoaded>::build(),
  0
);
static constexpr ecs::ComponentDesc ri_extra_destroy_es_event_handler_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
//start of 1 rq components at [1]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
//start of 1 no components at [2]
  {ECS_HASH("undestroyableRiExtra"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void ri_extra_destroy_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<CmdDestroyRendinst>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    ri_extra_destroy_es_event_handler(static_cast<const CmdDestroyRendinst&>(evt)
        , ECS_RO_COMP(ri_extra_destroy_es_event_handler_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc ri_extra_destroy_es_event_handler_es_desc
(
  "ri_extra_destroy_es",
  "prog/daNetGame/game/riDestrES.cpp.inl",
  ecs::EntitySystemOps(nullptr, ri_extra_destroy_es_event_handler_all_events),
  empty_span(),
  make_span(ri_extra_destroy_es_event_handler_comps+0, 1)/*ro*/,
  make_span(ri_extra_destroy_es_event_handler_comps+1, 1)/*rq*/,
  make_span(ri_extra_destroy_es_event_handler_comps+2, 1)/*no*/,
  ecs::EventSetBuilder<CmdDestroyRendinst>::build(),
  0
,"server");
static constexpr ecs::ComponentDesc destroyed_ladder_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ladder__sceneIndex"), ecs::ComponentTypeInfo<uint32_t>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc destroyed_ladder_ecs_query_desc
(
  "destroyed_ladder_ecs_query",
  make_span(destroyed_ladder_ecs_query_comps+0, 1)/*rw*/,
  make_span(destroyed_ladder_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline ecs::QueryCbResult destroyed_ladder_ecs_query(Callable function)
{
  return perform_query(g_entity_mgr, destroyed_ladder_ecs_query_desc.getHandle(),
    ecs::stoppable_query_cb_t([&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(destroyed_ladder_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(destroyed_ladder_ecs_query_comps, "ladder__sceneIndex", uint32_t)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    })
  );
}
static constexpr ecs::ComponentDesc destroyable_ents_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 rq components at [2]
  {ECS_HASH("destroyable_with_rendinst"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc destroyable_ents_ecs_query_desc
(
  "destroyable_ents_ecs_query",
  empty_span(),
  make_span(destroyable_ents_ecs_query_comps+0, 2)/*ro*/,
  make_span(destroyable_ents_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void destroyable_ents_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, destroyable_ents_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(destroyable_ents_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(destroyable_ents_ecs_query_comps, "transform", TMatrix)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc riextra_eid_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra__destrFx"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ri_extra__isBeingReplaced"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ri_extra__destroyed"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [4]
  {ECS_HASH("isRendinstDestr"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc riextra_eid_ecs_query_desc
(
  "ridestr::riextra_eid_ecs_query",
  empty_span(),
  make_span(riextra_eid_ecs_query_comps+0, 4)/*ro*/,
  make_span(riextra_eid_ecs_query_comps+4, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void ridestr::riextra_eid_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, riextra_eid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          if ( !(!ECS_RO_COMP_OR(riextra_eid_ecs_query_comps, "ri_extra__destroyed", bool( false))) )
            return;
          function(
              ECS_RO_COMP(riextra_eid_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP_OR(riextra_eid_ecs_query_comps, "ri_extra__destrFx", bool(true))
            , ECS_RO_COMP_OR(riextra_eid_ecs_query_comps, "ri_extra__isBeingReplaced", bool(false))
            );

        }
      }
  );
}
static constexpr ecs::ComponentDesc players_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("possessed"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("connid"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("disconnected"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [3]
  {ECS_HASH("player"), ecs::ComponentTypeInfo<ecs::auto_type>()},
//start of 1 no components at [4]
  {ECS_HASH("playerIsBot"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc players_ecs_query_desc
(
  "players_ecs_query",
  empty_span(),
  make_span(players_ecs_query_comps+0, 3)/*ro*/,
  make_span(players_ecs_query_comps+3, 1)/*rq*/,
  make_span(players_ecs_query_comps+4, 1)/*no*/);
template<typename Callable>
inline void players_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, players_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if ( !(!ECS_RO_COMP(players_ecs_query_comps, "disconnected", bool)) )
            continue;
          function(
              ECS_RO_COMP(players_ecs_query_comps, "possessed", ecs::EntityId)
            , ECS_RO_COMP(players_ecs_query_comps, "connid", int)
            );

        }while (++comp != compE);
      }
  );
}
static constexpr ecs::component_t riOffender_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t ri_extra__destroyed_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
static constexpr ecs::component_t ri_extra__isBeingReplaced_get_type(){return ecs::ComponentTypeInfo<bool>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
