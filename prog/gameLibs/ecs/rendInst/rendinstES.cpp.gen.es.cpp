#include "rendinstES.cpp.inl"
ECS_DEF_PULL_VAR(rendinst);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc rigrid_debug_pos_es_comps[] =
{
//start of 2 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("camera__active"), ecs::ComponentTypeInfo<bool>()}
};
static void rigrid_debug_pos_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
  {
    if ( !(ECS_RO_COMP(rigrid_debug_pos_es_comps, "camera__active", bool)) )
      continue;
    rigrid_debug_pos_es(*info.cast<ecs::UpdateStageInfoRenderDebug>()
    , ECS_RO_COMP(rigrid_debug_pos_es_comps, "transform", TMatrix)
    );
  }
  while (++comp != compE);
}
static ecs::EntitySystemDesc rigrid_debug_pos_es_es_desc
(
  "rigrid_debug_pos_es",
  "prog/gameLibs/ecs/rendInst/./rendinstES.cpp.inl",
  ecs::EntitySystemOps(rigrid_debug_pos_es_all),
  empty_span(),
  make_span(rigrid_debug_pos_es_comps+0, 2)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoRenderDebug::STAGE)
,"dev,render",nullptr,"*");
static constexpr ecs::ComponentDesc riextra_spawn_ri_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static void riextra_spawn_ri_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    riextra_spawn_ri_es_event_handler(evt
        , ECS_RW_COMP(riextra_spawn_ri_es_event_handler_comps, "ri_extra", RiExtraComponent)
    , ECS_RO_COMP(riextra_spawn_ri_es_event_handler_comps, "eid", ecs::EntityId)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc riextra_spawn_ri_es_event_handler_es_desc
(
  "riextra_spawn_ri_es",
  "prog/gameLibs/ecs/rendInst/./rendinstES.cpp.inl",
  ecs::EntitySystemOps(nullptr, riextra_spawn_ri_es_event_handler_all_events),
  make_span(riextra_spawn_ri_es_event_handler_comps+0, 1)/*rw*/,
  make_span(riextra_spawn_ri_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc riextra_destroyed_es_event_handler_comps[] =
{
//start of 4 ro components at [0]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
  {ECS_HASH("ri_extra__destroyed"), ecs::ComponentTypeInfo<bool>()},
  {ECS_HASH("ri_extra__riCellIdx"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ri_extra__riOffset"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL}
};
static void riextra_destroyed_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    riextra_destroyed_es_event_handler(evt
        , ECS_RO_COMP(riextra_destroyed_es_event_handler_comps, "ri_extra", RiExtraComponent)
    , ECS_RO_COMP(riextra_destroyed_es_event_handler_comps, "ri_extra__destroyed", bool)
    , ECS_RO_COMP_OR(riextra_destroyed_es_event_handler_comps, "ri_extra__riCellIdx", int(-1))
    , ECS_RO_COMP_OR(riextra_destroyed_es_event_handler_comps, "ri_extra__riOffset", int(-1))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc riextra_destroyed_es_event_handler_es_desc
(
  "riextra_destroyed_es",
  "prog/gameLibs/ecs/rendInst/./rendinstES.cpp.inl",
  ecs::EntitySystemOps(nullptr, riextra_destroyed_es_event_handler_all_events),
  empty_span(),
  make_span(riextra_destroyed_es_event_handler_comps+0, 4)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityDestroyed,
                       ecs::EventComponentsDisappear>::build(),
  0
);
static constexpr ecs::ComponentDesc rendinst_track_move_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
//start of 5 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("ri_extra__velocity"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ri_extra__omega"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("door_ri_extra__handles"), ecs::ComponentTypeInfo<ecs::UInt64List>(), ecs::CDF_OPTIONAL}
};
static void rendinst_track_move_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    rendinst_track_move_es_event_handler(evt
        , ECS_RO_COMP(rendinst_track_move_es_event_handler_comps, "eid", ecs::EntityId)
    , ECS_RW_COMP(rendinst_track_move_es_event_handler_comps, "ri_extra", RiExtraComponent)
    , ECS_RO_COMP(rendinst_track_move_es_event_handler_comps, "transform", TMatrix)
    , ECS_RO_COMP_OR(rendinst_track_move_es_event_handler_comps, "ri_extra__velocity", Point3(Point3(0, 0, 0)))
    , ECS_RO_COMP_OR(rendinst_track_move_es_event_handler_comps, "ri_extra__omega", Point3(Point3(0, 0, 0)))
    , ECS_RO_COMP_PTR(rendinst_track_move_es_event_handler_comps, "door_ri_extra__handles", ecs::UInt64List)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc rendinst_track_move_es_event_handler_es_desc
(
  "rendinst_track_move_es",
  "prog/gameLibs/ecs/rendInst/./rendinstES.cpp.inl",
  ecs::EntitySystemOps(nullptr, rendinst_track_move_es_event_handler_all_events),
  make_span(rendinst_track_move_es_event_handler_comps+0, 1)/*rw*/,
  make_span(rendinst_track_move_es_event_handler_comps+1, 5)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  0
,nullptr,"transform");
static constexpr ecs::ComponentDesc rendinst_move_es_event_handler_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL},
//start of 1 ro components at [3]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()}
};
static void rendinst_move_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    rendinst_move_es_event_handler(evt
        , ECS_RW_COMP(rendinst_move_es_event_handler_comps, "ri_extra", RiExtraComponent)
    , ECS_RO_COMP(rendinst_move_es_event_handler_comps, "transform", TMatrix)
    , ECS_RW_COMP_PTR(rendinst_move_es_event_handler_comps, "ri_extra__bboxMin", Point3)
    , ECS_RW_COMP_PTR(rendinst_move_es_event_handler_comps, "ri_extra__bboxMax", Point3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc rendinst_move_es_event_handler_es_desc
(
  "rendinst_move_es",
  "prog/gameLibs/ecs/rendInst/./rendinstES.cpp.inl",
  ecs::EntitySystemOps(nullptr, rendinst_move_es_event_handler_all_events),
  make_span(rendinst_move_es_event_handler_comps+0, 3)/*rw*/,
  make_span(rendinst_move_es_event_handler_comps+3, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventRendinstsLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc rendinst_with_handle_move_es_event_handler_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("ri_extra__bboxMin"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ri_extra__bboxMax"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL},
//start of 2 ro components at [2]
  {ECS_HASH("ri_extra__handle"), ecs::ComponentTypeInfo<rendinst::riex_handle_t>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 1 no components at [4]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()}
};
static void rendinst_with_handle_move_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    rendinst_with_handle_move_es_event_handler(evt
        , ECS_RO_COMP(rendinst_with_handle_move_es_event_handler_comps, "ri_extra__handle", rendinst::riex_handle_t)
    , ECS_RO_COMP(rendinst_with_handle_move_es_event_handler_comps, "transform", TMatrix)
    , ECS_RW_COMP_PTR(rendinst_with_handle_move_es_event_handler_comps, "ri_extra__bboxMin", Point3)
    , ECS_RW_COMP_PTR(rendinst_with_handle_move_es_event_handler_comps, "ri_extra__bboxMax", Point3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc rendinst_with_handle_move_es_event_handler_es_desc
(
  "rendinst_with_handle_move_es",
  "prog/gameLibs/ecs/rendInst/./rendinstES.cpp.inl",
  ecs::EntitySystemOps(nullptr, rendinst_with_handle_move_es_event_handler_all_events),
  make_span(rendinst_with_handle_move_es_event_handler_comps+0, 2)/*rw*/,
  make_span(rendinst_with_handle_move_es_event_handler_comps+2, 2)/*ro*/,
  empty_span(),
  make_span(rendinst_with_handle_move_es_event_handler_comps+4, 1)/*no*/,
  ecs::EventSetBuilder<EventRendinstsLoaded,
                       ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
static constexpr ecs::ComponentDesc del_ri_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
//start of 3 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("replication"), ecs::ComponentTypeInfo<net::Object>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("ri_extra__isBeingReplaced"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc del_ri_ecs_query_desc
(
  "del_ri_ecs_query",
  make_span(del_ri_ecs_query_comps+0, 1)/*rw*/,
  make_span(del_ri_ecs_query_comps+1, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void del_ri_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, del_ri_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(del_ri_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(del_ri_ecs_query_comps, "ri_extra", RiExtraComponent)
            , ECS_RO_COMP_PTR(del_ri_ecs_query_comps, "replication", net::Object)
            , ECS_RO_COMP_OR(del_ri_ecs_query_comps, "ri_extra__isBeingReplaced", bool(false))
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc restore_ri_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ri_extra"), ecs::ComponentTypeInfo<RiExtraComponent>()},
//start of 1 ro components at [1]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()}
};
static ecs::CompileTimeQueryDesc restore_ri_ecs_query_desc
(
  "restore_ri_ecs_query",
  make_span(restore_ri_ecs_query_comps+0, 1)/*rw*/,
  make_span(restore_ri_ecs_query_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void restore_ri_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, restore_ri_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(restore_ri_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RW_COMP(restore_ri_ecs_query_comps, "ri_extra", RiExtraComponent)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc check_extra_authority_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("ri_extra__destroyed"), ecs::ComponentTypeInfo<bool>()},
//start of 1 ro components at [1]
  {ECS_HASH("ri_extra__isBeingReplaced"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 1 rq components at [2]
  {ECS_HASH("riExtraAuthority"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc check_extra_authority_ecs_query_desc
(
  "check_extra_authority_ecs_query",
  make_span(check_extra_authority_ecs_query_comps+0, 1)/*rw*/,
  make_span(check_extra_authority_ecs_query_comps+1, 1)/*ro*/,
  make_span(check_extra_authority_ecs_query_comps+2, 1)/*rq*/,
  empty_span());
template<typename Callable>
inline void check_extra_authority_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, check_extra_authority_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(check_extra_authority_ecs_query_comps, "ri_extra__destroyed", bool)
            , components.manager()
            , ECS_RO_COMP_OR(check_extra_authority_ecs_query_comps, "ri_extra__isBeingReplaced", bool(false))
            );

        }
    }
  );
}
