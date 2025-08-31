// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "navMeshPhysProxyES.cpp.inl"
ECS_DEF_PULL_VAR(navMeshPhysProxy);
#include <daECS/core/internal/performQuery.h>
//static constexpr ecs::ComponentDesc clear_navmeshproxy_collision_es_event_handler_comps[] ={};
static void clear_navmeshproxy_collision_es_event_handler_all_events(const ecs::Event &__restrict evt, const ecs::QueryView &__restrict components)
{
  G_UNUSED(components);
  G_FAST_ASSERT(evt.is<EventOnGameShutdown>());
  clear_navmeshproxy_collision_es_event_handler(static_cast<const EventOnGameShutdown&>(evt)
        );
}
static ecs::EntitySystemDesc clear_navmeshproxy_collision_es_event_handler_es_desc
(
  "clear_navmeshproxy_collision_es",
  "prog/daNetGame/phys/navMeshPhysProxyES.cpp.inl",
  ecs::EntitySystemOps(nullptr, clear_navmeshproxy_collision_es_event_handler_all_events),
  empty_span(),
  empty_span(),
  empty_span(),
  empty_span(),
  ecs::EventSetBuilder<EventOnGameShutdown>::build(),
  0
);
static constexpr ecs::ComponentDesc navphys_collision_data_ecs_query_comps[] =
{
//start of 5 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("nphys_coll__capsuleRadius"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("nphys_coll__capsuleHeight"), ecs::ComponentTypeInfo<float>()},
  {ECS_HASH("navmesh_phys__currentWalkVelocity"), ecs::ComponentTypeInfo<Point3>()},
  {ECS_HASH("nphys__mass"), ecs::ComponentTypeInfo<float>()}
};
static ecs::CompileTimeQueryDesc navphys_collision_data_ecs_query_desc
(
  "navphys_collision_data_ecs_query",
  empty_span(),
  make_span(navphys_collision_data_ecs_query_comps+0, 5)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline bool navphys_collision_data_ecs_query(ecs::EntityId eid, Callable function)
{
  return perform_query(g_entity_mgr, eid, navphys_collision_data_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(navphys_collision_data_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(navphys_collision_data_ecs_query_comps, "nphys_coll__capsuleRadius", float)
            , ECS_RO_COMP(navphys_collision_data_ecs_query_comps, "nphys_coll__capsuleHeight", float)
            , ECS_RO_COMP(navphys_collision_data_ecs_query_comps, "navmesh_phys__currentWalkVelocity", Point3)
            , ECS_RO_COMP(navphys_collision_data_ecs_query_comps, "nphys__mass", float)
            );

        }
    }
  );
}
