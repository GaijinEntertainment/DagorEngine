//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <vecmath/dag_vecMath.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_function.h>
#include <daECS/core/ecsHash.h>
#include <daECS/core/entityId.h>
#include <daECS/core/componentType.h>
#include <memory/dag_framemem.h>
#include <math/dag_bounds3.h>
#include <grid/spatialHashGridImpl.h>

typedef eastl::fixed_vector<ecs::EntityId, 64, true, framemem_allocator> TempGridEntities;
typedef eastl::vector<const GridHolder *> GridHolders;
ECS_DECLARE_RELOCATABLE_TYPE(GridHolders);

const float MAX_VALID_BOUNDING_RADIUS = 64.f; // logerr anomalous large objects

enum class GridEntCheck
{
  POS, // Check is entity position (center of collres.vBoundingSphere) inside bounding.

  BOUNDING // Check is entity collres.vBoundingSphere intersects bounding.
           // You don't need to extend bounding manually to catch entities with position outside of bounding. It's done automatically.
           // BOUNDING check is fast as POS check, but query with BOUNDING option possibly iterating more grid cells and may be a bit
           // slower. Always use BOUNDING option if you need any entity geometry checks on found objects. POS is only for simple
           // position inclusion checks. Hint: you can query grid by bsphere with zero radius, it's valid case with BOUNDING option.
};

// sizeof = 48
struct GridObjComponent : public GridObject
{
  ecs::EntityId eid;
  bool inserted;
  bool hidden;
  uint16_t allocatorKey;
  GridHolder *ownerGrid;
  GridObjComponent(const ecs::EntityManager &mgr, ecs::EntityId eid_);
  ~GridObjComponent();
  vec3f getPos() const { return GridObject::wbsph; };
  BSphere3 getBSphere() const
  {
    BSphere3 bsph;
    v_stu(&bsph.c.x, GridObject::wbsph);
    bsph.r2 = bsph.r * bsph.r;
    return bsph;
  }
  void updatePos(vec4f new_pos, float new_bounding_rad);
  void removeFromGrid();
};
ECS_DECLARE_BOXED_TYPE(GridObjComponent);

GridHolder *find_grid_holder(ecs::EntityId gridh_ent);
GridHolder *find_grid_holder(uint32_t grid_name_hash);
GridHolder *find_grid_holder(ecs::HashedConstString grid_name_hash);
inline const GridHolder *find_grid_holder(const GridHolder *gh) { return gh; };
inline const GridHolder *find_grid_holder(const GridHolder &gh) { return &gh; };
void for_each_grid_holder(const eastl::function<void(const GridHolder &)> &cb);

inline const GridObjComponent *to_comp(const GridObject *obj) { return static_cast<const GridObjComponent *>(obj); }

template <typename F>
ecs::EntityId grid_find_entity(const GridHolder &grid_holder, const BBox3 &bbox, GridEntCheck check_type, const F &pred)
{
  auto findFunc = check_type == GridEntCheck::POS ? grid_find_in_box_by_pos : grid_find_in_box_by_bounding;
  const GridObject *obj =
    findFunc(grid_holder, bbox, [&pred](const GridObject *obj) { return pred(to_comp(obj)->eid, to_comp(obj)->getPos()); });
  return obj ? to_comp(obj)->eid : ecs::INVALID_ENTITY_ID;
}

template <typename F>
ecs::EntityId grid_find_entity(const GridHolder &grid_holder, const BSphere3 &bsphere, GridEntCheck check_type, const F &pred)
{
  auto findFunc = check_type == GridEntCheck::POS ? grid_find_in_sphere_by_pos : grid_find_in_sphere_by_bounding;
  const GridObject *obj = findFunc(grid_holder, bsphere.c, bsphere.r,
    [&pred](const GridObject *obj) { return pred(to_comp(obj)->eid, to_comp(obj)->getPos()); });
  return obj ? to_comp(obj)->eid : ecs::INVALID_ENTITY_ID;
}

template <typename F>
ecs::EntityId grid_find_entity(const GridHolder &grid_holder, const Point3 &from, const Point3 &dir, float len, float radius,
  GridEntCheck check_type, const F &pred)
{
  auto findFunc = check_type == GridEntCheck::POS ? grid_find_in_capsule_by_pos : grid_find_in_capsule_by_bounding;
  const GridObject *obj = findFunc(grid_holder, from, dir, len, radius,
    [&pred](const GridObject *obj) { return pred(to_comp(obj)->eid, to_comp(obj)->getPos()); });
  return obj ? to_comp(obj)->eid : ecs::INVALID_ENTITY_ID;
}

template <typename F>
ecs::EntityId grid_find_entity(const GridHolder &grid_holder, const TMatrix &transform, const BBox3 &bbox, GridEntCheck check_type,
  const F &pred)
{
  auto findFunc = check_type == GridEntCheck::POS ? grid_find_in_transformed_box_by_pos : grid_find_in_transformed_box_by_bounding;
  const GridObject *obj =
    findFunc(grid_holder, transform, bbox, [&pred](const GridObject *obj) { return pred(to_comp(obj)->eid, to_comp(obj)->getPos()); });
  return obj ? to_comp(obj)->eid : ecs::INVALID_ENTITY_ID;
}

/* Call cb(ecs::EntityId) for each entity in bounding.
   Params:
     * grid_id      = 32-bit ecs_hash or entity_id of grid holder
     * bound        = BBox3, bbox3f or BSphere3 bounding
     * check_type   = See GridEntCheck
     * cb           = Callback for each matching entity
*/
template <typename GridId, typename Bounding, typename F>
void for_each_entity_in_grid(GridId grid_id, const Bounding &bound, GridEntCheck check_type, const F &cb)
{
  if (const GridHolder *gridh = find_grid_holder(grid_id))
    grid_find_entity(*gridh, bound, check_type, [&cb](ecs::EntityId eid, vec3f pos) {
      cb(eid, pos);
      return false;
    });
}

/* Call cb(ecs::EntityId) for each entity in capsule.
   Params:
     * grid_id      = 32-bit ecs_hash or entity_id of grid holder
     * from         = Start point
     * dir          = Dir from start to end point of capsule axis
     * len          = Capsule axis len
     * radius       = Capsule radius
     * check_type   = See GridEntCheck
     * cb           = Callback for each matching entity
*/
template <typename GridId, typename F>
void for_each_entity_in_grid(GridId grid_id, const Point3 &from, const Point3 &dir, float len, float radius, GridEntCheck check_type,
  const F &cb)
{
  if (const GridHolder *gridh = find_grid_holder(grid_id))
    grid_find_entity(*gridh, from, dir, len, radius, check_type, [&cb](ecs::EntityId eid, vec3f pos) {
      cb(eid, pos);
      return false;
    });
}

/* Call cb(ecs::EntityId) for each entity in oriented bounding box (OBB).
   Params:
     * grid_id      = 32-bit ecs_hash or entity_id of grid holder
     * transform    = Orientation TMatrix
     * bbox         = BBox3 (AABB)
     * check_type   = See GridEntCheck
     * cb           = Callback for each matching entity
*/
template <typename GridId, typename F>
void for_each_entity_in_grid(GridId grid_id, const TMatrix &transform, const BBox3 &bbox, GridEntCheck check_type, const F &cb)
{
  if (const GridHolder *gridh = find_grid_holder(grid_id))
    grid_find_entity(*gridh, transform, bbox, check_type, [&cb](ecs::EntityId eid, vec3f pos) {
      cb(eid, pos);
      return false;
    });
}

/* Find entity in bounding using predicate function.
   Params:
     * grid_id      = 32-bit ecs_hash or entity_id of grid holder
     * bound        = BBox3, bbox3f or BSphere3 bounding
     * check_type   = See GridEntCheck
     * pred         = Predicate function for entities in bounding. Return true if entity matches additional conditions and you want
   stop searching Return:
     * eid of entity was found
*/
template <typename GridId, typename Bounding, typename F>
ecs::EntityId find_entity_in_grid(GridId grid_id, const Bounding &bound, GridEntCheck check_type, const F &pred)
{
  if (const GridHolder *gridh = find_grid_holder(grid_id))
    return grid_find_entity(*gridh, bound, check_type, pred);
  return ecs::INVALID_ENTITY_ID;
}

/* Find entity in capsule using predicate function. Use zero radius for ray trace.
   Params:
     * grid_id      = 32-bit ecs_hash or entity_id of grid holder
     * from         = Start point
     * dir          = Dir from start to end point of capsule axis
     * len          = Capsule axis len
     * radius       = Capsule radius
     * check_type   = See GridEntCheck
     * pred         = Predicate function for entities in bounding. Return true if entity matches additional conditions and you want
   stop searching Return:
     * eid of entity was found
*/
template <typename GridId, typename F>
ecs::EntityId find_entity_in_grid(GridId grid_id, const Point3 &from, const Point3 &dir, float len, float radius,
  GridEntCheck check_type, const F &pred)
{
  if (const GridHolder *gridh = find_grid_holder(grid_id))
    return grid_find_entity(*gridh, from, dir, len, radius, check_type, pred);
  return ecs::INVALID_ENTITY_ID;
}

/* Find entity in oriented bounding box (OBB) using predicate function.
   Params:
     * grid_id      = 32-bit ecs_hash or entity_id of grid holder
     * transform    = Orientation TMatrix
     * bbox         = BBox3 (AABB)
     * check_type   = See GridEntCheck
     * pred         = Predicate function for entities in bounding. Return true if entity matches additional conditions and you want
   stop searching Return:
     * eid of entity was found
*/
template <typename GridId, typename F>
ecs::EntityId find_entity_in_grid(GridId grid_id, const TMatrix &transform, const BBox3 &bbox, GridEntCheck check_type, const F &pred)
{
  if (const GridHolder *gridh = find_grid_holder(grid_id))
    return grid_find_entity(*gridh, transform, bbox, check_type, pred);
  return ecs::INVALID_ENTITY_ID;
}

/* Gather all entities in bounding. Use it only when you need all entities at time. Otherwise for_each_entity_in_grid would be faster.
   Params:
     * grid_id      = 32-bit ecs_hash or entity_id of grid holder
     * bound        = BBox3, bbox3f or BSphere3 bounding
     * check_type   = See GridEntCheck
     * entities     = Vector of all found entity eids
   Return:
     * true if 'entities' not empty
*/
template <typename GridId, typename Bounding>
bool gather_entities_in_grid(GridId grid_id, const Bounding &bound, GridEntCheck check_type, TempGridEntities &entities)
{
  if (const GridHolder *gridh = find_grid_holder(grid_id))
  {
    grid_find_entity(*gridh, bound, check_type, [&entities](ecs::EntityId eid, vec3f) {
      entities.push_back(eid);
      return false;
    });
    return !entities.empty();
  }
  return false;
}

/* Gather all entities in capsule. Use it only when you need all entities at time. Otherwise for_each_entity_in_grid would be faster.
   Params:
     * grid_id      = 32-bit ecs_hash or entity_id of grid holder
     * from         = Start point
     * dir          = Dir from start to end point of capsule axis
     * len          = Capsule axis len
     * radius       = Capsule radius
     * check_type   = See GridEntCheck
     * entities     = Vector of all found entity eids
   Return:
     * true if 'entities' not empty
*/
template <typename GridId>
bool gather_entities_in_grid(GridId grid_id, const Point3 &from, const Point3 &dir, float len, float radius, GridEntCheck check_type,
  TempGridEntities &entities)
{
  if (const GridHolder *gridh = find_grid_holder(grid_id))
  {
    grid_find_entity(*gridh, from, dir, len, radius, check_type, [&entities](ecs::EntityId eid, vec3f) {
      entities.push_back(eid);
      return false;
    });
    return !entities.empty();
  }
  return false;
}

/* Gather all entities in oriented bounding box (OBB). Use it only when you need all entities at time. Otherwise
   for_each_entity_in_grid would be faster. Params:
     * grid_id      = 32-bit ecs_hash or entity_id of grid holder
     * transform    = Orientation TMatrix
     * bbox         = BBox3 (AABB)
     * check_type   = See GridEntCheck
     * entities     = Vector of all found entity eids
   Return:
     * true if 'entities' not empty
*/
template <typename GridId>
bool gather_entities_in_grid(GridId grid_id, const TMatrix &transform, const BBox3 &bbox, GridEntCheck check_type,
  TempGridEntities &entities)
{
  if (const GridHolder *gridh = find_grid_holder(grid_id))
  {
    grid_find_entity(*gridh, transform, bbox, check_type, [&entities](ecs::EntityId eid, vec3f) {
      entities.push_back(eid);
      return false;
    });
    return !entities.empty();
  }
  return false;
}

/* Multi-holder versions */
template <typename GridId, size_t N, typename... Args>
void for_each_entity_in_grids(GridId (&grid_ids)[N], Args... args)
{
  const GridHolder *gridh[N];
  for (int i = 0; i < N; i++)
    gridh[i] = find_grid_holder(grid_ids[i]);
  for (int i = 0; i < N; i++)
    for_each_entity_in_grid(gridh[i], args...);
}

template <typename GridIdsContainer, typename... Args>
void for_each_entity_in_grids(const GridIdsContainer &grid_ids, Args... args)
{
  for (auto grid_id : grid_ids)
    for_each_entity_in_grid(find_grid_holder(grid_id), args...);
}

template <typename GridId, size_t N, typename... Args>
ecs::EntityId find_entity_in_grids(GridId (&grid_ids)[N], Args... args)
{
  for (int i = 0; i < N; i++)
    if (ecs::EntityId eid = find_entity_in_grid(find_grid_holder(grid_ids[i]), args...))
      return eid;
  return ecs::INVALID_ENTITY_ID;
}

template <typename GridId, size_t N, typename Bounding>
void gather_entities_in_grids(GridId (&grid_ids)[N], const Bounding &bound, GridEntCheck check_type, TempGridEntities &entities)
{
  const GridHolder *gridh[N];
  for (int i = 0; i < N; i++)
    gridh[i] = find_grid_holder(grid_ids[i]);
  for (int i = 0; i < N; i++)
    gather_entities_in_grid(gridh[i], bound, check_type, entities);
}

template <typename GridId, size_t N, typename Bounding>
void gather_entities_in_grids(GridId (&grid_ids)[N], const Point3 &from, const Point3 &dir, float len, float radius,
  GridEntCheck check_type, TempGridEntities &entities)
{
  const GridHolder *gridh[N];
  for (int i = 0; i < N; i++)
    gridh[i] = find_grid_holder(grid_ids[i]);
  for (int i = 0; i < N; i++)
    gather_entities_in_grid(gridh[i], from, dir, len, radius, check_type, entities);
}

template <typename GridId, size_t N>
void gather_entities_in_grids(GridId (&grid_ids)[N], const TMatrix &transform, const BBox3 &bbox, GridEntCheck check_type,
  TempGridEntities &entities)
{
  const GridHolder *gridh[N];
  for (int i = 0; i < N; i++)
    gridh[i] = find_grid_holder(grid_ids[i]);
  for (int i = 0; i < N; i++)
    gather_entities_in_grid(gridh[i], transform, bbox, check_type, entities);
}
