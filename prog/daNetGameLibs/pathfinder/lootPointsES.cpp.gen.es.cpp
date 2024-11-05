#include "lootPointsES.cpp.inl"
ECS_DEF_PULL_VAR(lootPoints);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc loot_points_es_event_handler_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("loot_points"), ecs::ComponentTypeInfo<ecs::Point3List>()},
//start of 7 ro components at [1]
  {ECS_HASH("loot_points__center"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("loot_points__radius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("loot_points__numPoints"), ecs::ComponentTypeInfo<int>()},
  {ECS_HASH("loot_points__blk"), ecs::ComponentTypeInfo<ecs::string>()},
  {ECS_HASH("loot_points__underRoof"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("loot_points__closeRange"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("loot_points__farRange"), ecs::ComponentTypeInfo<float>(), ecs::CDF_OPTIONAL}
};
static void loot_points_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<EventGameObjectsCreated>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    loot_points_es_event_handler(static_cast<const EventGameObjectsCreated&>(evt)
        , ECS_RW_COMP(loot_points_es_event_handler_comps, "loot_points", ecs::Point3List)
    , ECS_RO_COMP(loot_points_es_event_handler_comps, "loot_points__center", Point3)
    , ECS_RO_COMP(loot_points_es_event_handler_comps, "loot_points__radius", float)
    , ECS_RO_COMP(loot_points_es_event_handler_comps, "loot_points__numPoints", int)
    , ECS_RO_COMP(loot_points_es_event_handler_comps, "loot_points__blk", ecs::string)
    , ECS_RO_COMP_OR(loot_points_es_event_handler_comps, "loot_points__underRoof", bool(true))
    , ECS_RO_COMP_OR(loot_points_es_event_handler_comps, "loot_points__closeRange", float(5.f))
    , ECS_RO_COMP_OR(loot_points_es_event_handler_comps, "loot_points__farRange", float(10.f))
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc loot_points_es_event_handler_es_desc
(
  "loot_points_es",
  "prog/daNetGameLibs/pathfinder/./lootPointsES.cpp.inl",
  ecs::EntitySystemOps(nullptr, loot_points_es_event_handler_all_events),
  make_span(loot_points_es_event_handler_comps+0, 1)/*rw*/,
  make_span(loot_points_es_event_handler_comps+1, 7)/*ro*/,
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventGameObjectsCreated>::build(),
  0
);
