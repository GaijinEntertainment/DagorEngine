// Built with ECS codegen version 1.0
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "gridCollisionES.cpp.inl"
ECS_DEF_PULL_VAR(gridCollision);
#include <daECS/core/internal/performQuery.h>
static constexpr ecs::ComponentDesc collidable_grid_obj_eid_ecs_query_comps[] =
{
//start of 1 rw components at [0]
  {ECS_HASH("collision_physMatId"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL},
//start of 6 ro components at [1]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()},
  {ECS_HASH("havePairCollision"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("collisionGridTraceable"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("net_phys__currentStateVelocity"), ecs::ComponentTypeInfo<Point3>(), ecs::CDF_OPTIONAL},
//start of 1 no components at [7]
  {ECS_HASH("deadEntity"), ecs::ComponentTypeInfo<ecs::Tag>()}
};
static ecs::CompileTimeQueryDesc collidable_grid_obj_eid_ecs_query_desc
(
  "collidable_grid_obj_eid_ecs_query",
  make_span(collidable_grid_obj_eid_ecs_query_comps+0, 1)/*rw*/,
  make_span(collidable_grid_obj_eid_ecs_query_comps+1, 6)/*ro*/,
  empty_span(),
  make_span(collidable_grid_obj_eid_ecs_query_comps+7, 1)/*no*/);
template<typename Callable>
inline void collidable_grid_obj_eid_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, collidable_grid_obj_eid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(collidable_grid_obj_eid_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(collidable_grid_obj_eid_ecs_query_comps, "collres", CollisionResource)
            , ECS_RO_COMP(collidable_grid_obj_eid_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            , ECS_RO_COMP_OR(collidable_grid_obj_eid_ecs_query_comps, "havePairCollision", bool(true))
            , ECS_RO_COMP_OR(collidable_grid_obj_eid_ecs_query_comps, "collisionGridTraceable", bool(false))
            , ECS_RW_COMP_PTR(collidable_grid_obj_eid_ecs_query_comps, "collision_physMatId", int)
            , ECS_RO_COMP_PTR(collidable_grid_obj_eid_ecs_query_comps, "net_phys__currentStateVelocity", Point3)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc base_phys_grid_obj_eid_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("base_net_phys_ptr"), ecs::ComponentTypeInfo<BasePhysActorPtr>()},
  {ECS_HASH("havePairCollision"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL},
  {ECS_HASH("collisionGridTraceable"), ecs::ComponentTypeInfo<bool>(), ecs::CDF_OPTIONAL}
};
static ecs::CompileTimeQueryDesc base_phys_grid_obj_eid_ecs_query_desc
(
  "base_phys_grid_obj_eid_ecs_query",
  empty_span(),
  make_span(base_phys_grid_obj_eid_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void base_phys_grid_obj_eid_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, base_phys_grid_obj_eid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(base_phys_grid_obj_eid_ecs_query_comps, "base_net_phys_ptr", BasePhysActorPtr)
            , ECS_RO_COMP_OR(base_phys_grid_obj_eid_ecs_query_comps, "havePairCollision", bool(true))
            , ECS_RO_COMP_OR(base_phys_grid_obj_eid_ecs_query_comps, "collisionGridTraceable", bool(false))
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc trace_grid_object_eid_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()}
};
static ecs::CompileTimeQueryDesc trace_grid_object_eid_ecs_query_desc
(
  "trace_grid_object_eid_ecs_query",
  empty_span(),
  make_span(trace_grid_object_eid_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void trace_grid_object_eid_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, trace_grid_object_eid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(trace_grid_object_eid_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(trace_grid_object_eid_ecs_query_comps, "collres", CollisionResource)
            , ECS_RO_COMP(trace_grid_object_eid_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc trace_grid_object_by_capsule_eid_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()}
};
static ecs::CompileTimeQueryDesc trace_grid_object_by_capsule_eid_ecs_query_desc
(
  "trace_grid_object_by_capsule_eid_ecs_query",
  empty_span(),
  make_span(trace_grid_object_by_capsule_eid_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void trace_grid_object_by_capsule_eid_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, trace_grid_object_by_capsule_eid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(trace_grid_object_by_capsule_eid_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(trace_grid_object_by_capsule_eid_ecs_query_comps, "collres", CollisionResource)
            , ECS_RO_COMP(trace_grid_object_by_capsule_eid_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            );

        }
    }
  );
}
static constexpr ecs::ComponentDesc intersected_eid_ecs_query_comps[] =
{
//start of 3 ro components at [0]
  {ECS_HASH("transform"), ecs::ComponentTypeInfo<TMatrix>()},
  {ECS_HASH("collres"), ecs::ComponentTypeInfo<CollisionResource>()},
  {ECS_HASH("animchar"), ecs::ComponentTypeInfo<AnimV20::AnimcharBaseComponent>()}
};
static ecs::CompileTimeQueryDesc intersected_eid_ecs_query_desc
(
  "intersected_eid_ecs_query",
  empty_span(),
  make_span(intersected_eid_ecs_query_comps+0, 3)/*ro*/,
  empty_span(),
  empty_span());
template<typename Callable>
inline void intersected_eid_ecs_query(ecs::EntityId eid, Callable function)
{
  perform_query(g_entity_mgr, eid, intersected_eid_ecs_query_desc.getHandle(),
    [&function](const ecs::QueryView& __restrict components)
    {
        constexpr size_t comp = 0;
        {
          function(
              ECS_RO_COMP(intersected_eid_ecs_query_comps, "transform", TMatrix)
            , ECS_RO_COMP(intersected_eid_ecs_query_comps, "collres", CollisionResource)
            , ECS_RO_COMP(intersected_eid_ecs_query_comps, "animchar", AnimV20::AnimcharBaseComponent)
            );

        }
    }
  );
}
