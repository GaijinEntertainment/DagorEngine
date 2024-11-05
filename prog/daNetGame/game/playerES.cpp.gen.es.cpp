#include <daECS/core/internal/ltComponentList.h>
static constexpr ecs::component_t connid_get_type();
static ecs::LTComponentList connid_component(ECS_HASH("connid"), connid_get_type(), "prog/daNetGame/game/playerES.cpp.inl", "", 0);
static constexpr ecs::component_t player_get_type();
static ecs::LTComponentList player_component(ECS_HASH("player"), player_get_type(), "prog/daNetGame/game/playerES.cpp.inl", "", 0);
static constexpr ecs::component_t possessed_get_type();
static ecs::LTComponentList possessed_component(ECS_HASH("possessed"), possessed_get_type(), "prog/daNetGame/game/playerES.cpp.inl", "", 0);
static constexpr ecs::component_t possessedByPlr_get_type();
static ecs::LTComponentList possessedByPlr_component(ECS_HASH("possessedByPlr"), possessedByPlr_get_type(), "prog/daNetGame/game/playerES.cpp.inl", "", 0);
static constexpr ecs::component_t specTarget_get_type();
static ecs::LTComponentList specTarget_component(ECS_HASH("specTarget"), specTarget_get_type(), "prog/daNetGame/game/playerES.cpp.inl", "", 0);
static constexpr ecs::component_t team_get_type();
static ecs::LTComponentList team_component(ECS_HASH("team"), team_get_type(), "prog/daNetGame/game/playerES.cpp.inl", "players_eid_connection_ecs_query", 0);
static constexpr ecs::component_t transform_get_type();
static ecs::LTComponentList transform_component(ECS_HASH("transform"), transform_get_type(), "prog/daNetGame/game/playerES.cpp.inl", "", 0);
#include "playerES.cpp.inl"
ECS_DEF_PULL_VAR(player);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc players_search_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("player"), ecs::ComponentTypeInfo<game::Player>()},
//start of 1 ro components at [1]
  {ECS_HASH("userid"), ecs::ComponentTypeInfo<uint64_t>()}
};
static ecs::CompileTimeQueryDesc players_search_ecs_query_desc
(
  "players_search_ecs_query",
  make_span(players_search_ecs_query_comps+0, 1)/*rw*/,
  make_span(players_search_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void players_search_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, players_search_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RW_COMP(players_search_ecs_query_comps, "player", game::Player)
            , ECS_RO_COMP(players_search_ecs_query_comps, "userid", uint64_t)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    }
  );
}
static constexpr ecs::ComponentDesc players_search_by_platfrom_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("player"), ecs::ComponentTypeInfo<game::Player>()},
//start of 1 ro components at [1]
  {ECS_HASH("platformUid"), ecs::ComponentTypeInfo<eastl::string>()}
};
static ecs::CompileTimeQueryDesc players_search_by_platfrom_ecs_query_desc
(
  "players_search_by_platfrom_ecs_query",
  make_span(players_search_by_platfrom_ecs_query_comps+0, 1)/*rw*/,
  make_span(players_search_by_platfrom_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void players_search_by_platfrom_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, players_search_by_platfrom_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RW_COMP(players_search_by_platfrom_ecs_query_comps, "player", game::Player)
            , ECS_RO_COMP(players_search_by_platfrom_ecs_query_comps, "platformUid", eastl::string)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    }
  );
}
static constexpr ecs::ComponentDesc players_connection_ecs_query_comps[] =
{
//start of 1 ro components at [0]
  {ECS_HASH("disconnected"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [1]
  {ECS_HASH("player"), ecs::ComponentTypeInfo<game::Player>()},
//start of 1 no components at [2]
  {ECS_HASH("playerIsBot"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc players_connection_ecs_query_desc
(
  "players_connection_ecs_query",
  empty_span(),
  make_span(players_connection_ecs_query_comps+0, 1)/*ro*/,
  make_span(players_connection_ecs_query_comps+1, 1)/*rq*/,
  make_span(players_connection_ecs_query_comps+2, 1)/*no*/);
template<typename Callable>
inline void players_connection_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, players_connection_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(players_connection_ecs_query_comps, "disconnected", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc players_eid_connection_ecs_query_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("possessed"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("connid"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("disconnected"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [4]
  {ECS_HASH("player"), ecs::ComponentTypeInfo<game::Player>()}
};
static ecs::CompileTimeQueryDesc players_eid_connection_ecs_query_desc
(
  "players_eid_connection_ecs_query",
  empty_span(),
  make_span(players_eid_connection_ecs_query_comps+0, 4)/*ro*/,
  make_span(players_eid_connection_ecs_query_comps+4, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void players_eid_connection_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, players_eid_connection_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(players_eid_connection_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(players_eid_connection_ecs_query_comps, "possessed", ecs::EntityId)
            , ECS_RO_COMP(players_eid_connection_ecs_query_comps, "connid", int)
            , ECS_RO_COMP(players_eid_connection_ecs_query_comps, "disconnected", bool)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    }
  );
}
static constexpr ecs::ComponentDesc players_ecs_query_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("possessed"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("disconnected"), ecs::ComponentTypeInfo<bool>()},
//start of 1 rq components at [2]
  {ECS_HASH("player"), ecs::ComponentTypeInfo<game::Player>()}
};
static ecs::CompileTimeQueryDesc players_ecs_query_desc
(
  "players_ecs_query",
  empty_span(),
  make_span(players_ecs_query_comps+0, 2)/*ro*/,
  make_span(players_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void players_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, players_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          if (function(
              ECS_RO_COMP(players_ecs_query_comps, "possessed", ecs::EntityId)
            , ECS_RO_COMP(players_ecs_query_comps, "disconnected", bool)
            ) == ecs::QueryCbResult::Stop)
            return ecs::QueryCbResult::Stop;
        }while (++comp != compE);
          return ecs::QueryCbResult::Continue;
    }
  );
}
static constexpr ecs::component_t connid_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t player_get_type(){return ecs::ComponentTypeInfo<game::Player>::type; }
static constexpr ecs::component_t possessed_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t possessedByPlr_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t specTarget_get_type(){return ecs::ComponentTypeInfo<ecs::EntityId>::type; }
static constexpr ecs::component_t team_get_type(){return ecs::ComponentTypeInfo<int>::type; }
static constexpr ecs::component_t transform_get_type(){return ecs::ComponentTypeInfo<TMatrix>::type; }
