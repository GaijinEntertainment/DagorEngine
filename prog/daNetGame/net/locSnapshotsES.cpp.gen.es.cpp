#include "locSnapshotsES.cpp.inl"
ECS_DEF_PULL_VAR(locSnapshots);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc send_transform_snapshots_es_comps[] ={};
static void send_transform_snapshots_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  G_UNUSED(components);
    send_transform_snapshots_es(*info.cast<ecs::UpdateStageInfoAct>());
}
static ecs::EntitySystemDesc send_transform_snapshots_es_es_desc
(
  "send_transform_snapshots_es",
  "prog/daNetGame/net/locSnapshotsES.cpp.inl",
  ecs::EntitySystemOps(send_transform_snapshots_es_all),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"net,server",nullptr,"*");
static constexpr ecs::ComponentDesc cleanup_loc_snapshots_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("loc_snapshots__snapshotData"), ecs::ComponentTypeInfo<LocSnapshotsList>()},
//start of 1 ro components at [1]
  {ECS_HASH("loc_snapshots__interpTimeOffset"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [2]
  {ECS_HASH("loc_snapshots__dynamicInterpTime"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void cleanup_loc_snapshots_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    cleanup_loc_snapshots_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(cleanup_loc_snapshots_es_comps, "loc_snapshots__snapshotData", LocSnapshotsList)
    , ECS_RO_COMP_OR(cleanup_loc_snapshots_es_comps, "loc_snapshots__interpTimeOffset", float(0.15f))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc cleanup_loc_snapshots_es_es_desc
(
  "cleanup_loc_snapshots_es",
  "prog/daNetGame/net/locSnapshotsES.cpp.inl",
  ecs::EntitySystemOps(cleanup_loc_snapshots_es_all),
  make_span(cleanup_loc_snapshots_es_comps+0, 1)/*rw*/,
  make_span(cleanup_loc_snapshots_es_comps+1, 1)/*ro*/,
  empty_span(),
  make_span(cleanup_loc_snapshots_es_comps+2, 1)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"gameClient",nullptr,"*");
static constexpr ecs::ComponentDesc interp_loc_snapshots_es_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
//start of 3 ro components at [1]
  {ECS_HASH("loc_snapshots__snapshotData"), ecs::ComponentTypeInfo<LocSnapshotsList>()},
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("loc_snapshots__interpTimeOffset"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
//start of 2 no components at [4]
  {ECS_HASH("loc_snapshots__dontUpdate"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("loc_snapshots__dynamicInterpTime"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static void interp_loc_snapshots_es_all(const ecs::UpdateStageInfo &__restrict info, const ecs::QueryView & __restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE);
  do
    interp_loc_snapshots_es(*info.cast<ecs::UpdateStageInfoAct>()
    , ECS_RW_COMP(interp_loc_snapshots_es_comps, "transform", TMatrix)
    , ECS_RO_COMP(interp_loc_snapshots_es_comps, "loc_snapshots__snapshotData", LocSnapshotsList)
    , ECS_RO_COMP(interp_loc_snapshots_es_comps, "eid", ecs::EntityId)
    , ECS_RO_COMP_OR(interp_loc_snapshots_es_comps, "loc_snapshots__interpTimeOffset", float(0.15f))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc interp_loc_snapshots_es_es_desc
(
  "interp_loc_snapshots_es",
  "prog/daNetGame/net/locSnapshotsES.cpp.inl",
  ecs::EntitySystemOps(interp_loc_snapshots_es_all),
  make_span(interp_loc_snapshots_es_comps+0, 1)/*rw*/,
  make_span(interp_loc_snapshots_es_comps+1, 3)/*ro*/,
  empty_span(),
  make_span(interp_loc_snapshots_es_comps+4, 2)/*no*/,
  ecs::EventSetBuilder<>::build(),
  (1<<ecs::UpdateStageInfoAct::STAGE)
,"gameClient",nullptr,"before_animchar_update_sync");
static constexpr ecs::ComponentDesc init_snapshot_send_period_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("loc_snapshots__sendPeriod"), ecs::ComponentTypeInfo<float>()},
//start of 1 ro components at [1]
  {ECS_HASH("loc_snapshots__sendRate"), ecs::ComponentTypeInfo<float>()}
};
static void init_snapshot_send_period_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    init_snapshot_send_period_es_event_handler(evt
        , ECS_RO_COMP(init_snapshot_send_period_es_event_handler_comps, "loc_snapshots__sendRate", float)
    , ECS_RW_COMP(init_snapshot_send_period_es_event_handler_comps, "loc_snapshots__sendPeriod", float)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc init_snapshot_send_period_es_event_handler_es_desc
(
  "init_snapshot_send_period_es",
  "prog/daNetGame/net/locSnapshotsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, init_snapshot_send_period_es_event_handler_all_events),
  make_span(init_snapshot_send_period_es_event_handler_comps+0, 1)/*rw*/,
  make_span(init_snapshot_send_period_es_event_handler_comps+1, 1)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<ecs::EventEntityCreated,
                       ecs::EventComponentsAppear>::build(),
  0
);
//static constexpr ecs::ComponentDesc rcv_loc_snapshots_es_event_handler_comps[] ={};
static void rcv_loc_snapshots_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<TransformSnapshots>());
  rcv_loc_snapshots_es_event_handler(static_cast<const TransformSnapshots&>(evt)
        );
}
static ecs::EntitySystemDesc rcv_loc_snapshots_es_event_handler_es_desc
(
  "rcv_loc_snapshots_es",
  "prog/daNetGame/net/locSnapshotsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, rcv_loc_snapshots_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<TransformSnapshots>::build(),
  0
,"gameClient");
static constexpr ecs::ComponentDesc gather_transform_snapshot_entities_ecs_query_comps[] =
{
//start of 2 rw components at [0]
  {ECS_HASH("loc_snapshots__sendAtTime"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("loc_snapshots__blink"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
//start of 4 ro components at [2]
  {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("loc_snapshots__sendPeriod"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("loc_snapshots__scaledTransform"), ecs::ComponentTypeInfo<ecs::Tag>(), ecs::CDF_OPTIONAL},
//start of 2 no components at [6]
  {ECS_HASH("deadEntity"), ecs::ComponentTypeInfo<ecs::Tag>()},
  {ECS_HASH("loc_snaphots__disabled"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc gather_transform_snapshot_entities_ecs_query_desc
(
  "gather_transform_snapshot_entities_ecs_query",
  make_span(gather_transform_snapshot_entities_ecs_query_comps+0, 2)/*rw*/,
  make_span(gather_transform_snapshot_entities_ecs_query_comps+2, 4)/*ro*/,
  empty_span(),
  make_span(gather_transform_snapshot_entities_ecs_query_comps+6, 2)/*no*/);
template<typename Callable>
inline void gather_transform_snapshot_entities_ecs_query(Callable function)
{
  perform_query(g_entity_mgr, gather_transform_snapshot_entities_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        auto comp = components.begin(), compE = components.end(); G_ASSERT(comp != compE); do
        {
          function(
              ECS_RO_COMP(gather_transform_snapshot_entities_ecs_query_comps, "eid", ecs::EntityId)
            , ECS_RO_COMP(gather_transform_snapshot_entities_ecs_query_comps, "transform", TMatrix)
            , ECS_RW_COMP(gather_transform_snapshot_entities_ecs_query_comps, "loc_snapshots__sendAtTime", float)
            , ECS_RO_COMP(gather_transform_snapshot_entities_ecs_query_comps, "loc_snapshots__sendPeriod", float)
            , ECS_RO_COMP_PTR(gather_transform_snapshot_entities_ecs_query_comps, "loc_snapshots__scaledTransform", ecs::Tag)
            , ECS_RW_COMP_PTR(gather_transform_snapshot_entities_ecs_query_comps, "loc_snapshots__blink", bool)
            );

        }while (++comp != compE);
    }
  );
}
static constexpr ecs::ComponentDesc apply_transform_snapshot_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("loc_snapshots__snapshotData"), ecs::ComponentTypeInfo<LocSnapshotsList>()}
};
static ecs::CompileTimeQueryDesc apply_transform_snapshot_ecs_query_desc
(
  "apply_transform_snapshot_ecs_query",
  make_span(apply_transform_snapshot_ecs_query_comps+0, 1)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span());
template<typename Callable>
inline void apply_transform_snapshot_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, apply_transform_snapshot_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RW_COMP(apply_transform_snapshot_ecs_query_comps, "loc_snapshots__snapshotData", LocSnapshotsList)
            );

        }
    }
  );
}
