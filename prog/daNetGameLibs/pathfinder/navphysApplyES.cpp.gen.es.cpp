#include "navphysApplyES.cpp.inl"
ECS_DEF_PULL_VAR(navphysApply);
//built with ECS codegen version 1.0
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc navphys_apply_es_event_handler_comps[] =
{
//start of 3 rw components at [0]
  {ECS_HASH("navmesh_phys__currentPoly"), ecs::ComponentTypeInfo<int64_t>()},
  {ECS_HASH("navmesh_phys__currentPos"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("navmesh_phys__currentWalkVelocity"), ecs::ComponentTypeInfo<Point3>()}
};
static void navphys_apply_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_FAST_ASSERT(evt.is<CmdNavPhysCollisionApply>());
  auto comp = components.begin(), compE = components.end(); G_ASSERT(comp!=compE); do
    navphys_apply_es_event_handler(static_cast<const CmdNavPhysCollisionApply&>(evt)
        , ECS_RW_COMP(navphys_apply_es_event_handler_comps, "navmesh_phys__currentPoly", int64_t)
    , ECS_RW_COMP(navphys_apply_es_event_handler_comps, "navmesh_phys__currentPos", Point3)
    , ECS_RW_COMP(navphys_apply_es_event_handler_comps, "navmesh_phys__currentWalkVelocity", Point3)
    );
  while (++comp != compE);
}
static ecs::EntitySystemDesc navphys_apply_es_event_handler_es_desc
(
  "navphys_apply_es",
  "prog/daNetGameLibs/pathfinder/./navphysApplyES.cpp.inl",
  ecs::EntitySystemOps(nullptr, navphys_apply_es_event_handler_all_events),
  make_span(navphys_apply_es_event_handler_comps+0, 3)/*rw*/,
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<CmdNavPhysCollisionApply>::build(),
  0
,nullptr,nullptr,"*");
