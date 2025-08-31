// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gridCollision.h"
#include <ecs/anim/anim.h>
#include <ecs/phys/collRes.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <gamePhys/collision/collisionLib.h>
#include <phys/netPhys.h>

template <typename Callable>
static void collidable_grid_obj_eid_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
static void base_phys_grid_obj_eid_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
static void trace_grid_object_eid_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
static void trace_grid_object_by_capsule_eid_ecs_query(ecs::EntityId eid, Callable c);
template <typename Callable>
static void intersected_eid_ecs_query(ecs::EntityId eid, Callable c);

static uint32_t main_grid_types[] = {ECS_HASH("humans").hash, ECS_HASH("vehicles").hash};


bool grid_trace_main_entities(const Point3 &from, const Point3 &dir, float &t, Point3 &out_vel, int ignore_id, int ray_mat_id)
{
  out_vel.zero();
  bool res = false;

  for_each_entity_in_grids(main_grid_types, from, dir, t, 0.f /* radius */, GridEntCheck::BOUNDING, [&](ecs::EntityId eid, vec3f) {
    if (eid == ecs::EntityId(ignore_id))
      return;
    collidable_grid_obj_eid_ecs_query(eid,
      [&](ECS_REQUIRE_NOT(ecs::Tag deadEntity) const TMatrix &transform, const CollisionResource &collres,
        const AnimV20::AnimcharBaseComponent &animchar, bool havePairCollision = true, bool collisionGridTraceable = false,
        int *collision_physMatId = nullptr, const Point3 *net_phys__currentStateVelocity = nullptr) {
        if (havePairCollision || collisionGridTraceable)
        {
          int gridObjectPhysMatId = collision_physMatId ? *collision_physMatId : PHYSMAT_INVALID;
          bool isHit = false;
          int outMatId, outNodeId;
          const GeomNodeTree &nodeTree = animchar.getNodeTree();
          Point3 *pNorm = nullptr;
          if (ray_mat_id != PHYSMAT_INVALID)
          {
            if (gridObjectPhysMatId == PHYSMAT_INVALID || PhysMat::isMaterialsCollide(gridObjectPhysMatId, ray_mat_id))
              isHit = collres.traceRay(transform, &nodeTree, from, dir, t, pNorm, outMatId, {}, ray_mat_id);
          }
          else
            isHit = collres.traceRay(transform, &nodeTree, from, dir, t, pNorm, outMatId, outNodeId);

          if (isHit && net_phys__currentStateVelocity)
            out_vel = *net_phys__currentStateVelocity;

          res |= isHit;
        }
      });
  });

  return res;
}

void grid_process_main_entities_collision_objects_on_ray(
  const Point3 &from, const Point3 &to, float radius, GameObjectsCollisionsCb coll_cb)
{
  Point3 dir = to - from;
  const float dist = length(dir);
  dir *= safeinv(dist);
  for_each_entity_in_grids(main_grid_types, from, dir, dist, radius, GridEntCheck::BOUNDING, [&](ecs::EntityId eid, vec3f) {
    base_phys_grid_obj_eid_ecs_query(eid,
      [&](const BasePhysActorPtr &base_net_phys_ptr, bool havePairCollision = true, bool collisionGridTraceable = false) {
        if ((havePairCollision || collisionGridTraceable) && base_net_phys_ptr)
          for (CollisionObject obj : base_net_phys_ptr->getPhys().getCollisionObjects())
            coll_cb(obj);
      });
  });
}

// -V::657 Suppress PVS warning: It's odd that this function always returns one and the same value. WTF??
bool trace_entities_in_grid(uint32_t grid_hash,
  const Point3 &from,
  const Point3 &dir,
  float &t,
  ecs::EntityId ignore_eid,
  IntersectedEntities &intersected_entities,
  SortIntersections do_sort)
{
  const float initialT = t;
  for_each_entity_in_grid(grid_hash, from, dir, t, 0.f /* radius */, GridEntCheck::BOUNDING, [&](ecs::EntityId eid, vec3f) {
    if (eid == ignore_eid)
      return;
    trace_grid_object_eid_ecs_query(eid,
      [&](const TMatrix &transform, const CollisionResource &collres, const AnimV20::AnimcharBaseComponent &animchar) {
        CollResIntersectionsType isects;
        if (collres.traceRay(transform, &animchar.getNodeTree(), from, dir, initialT, isects, true))
        {
          for (auto &isect : isects)
          {
            IntersectedEntity &ent = *(IntersectedEntity *)intersected_entities.push_back_uninitialized();
            ent.eid = eid;
            ent.t = isect.intersectionT;
            ent.collNodeId = isect.collisionNodeId;
            ent.norm = isect.normal;
            ent.pos = isect.intersectionPos;
            ent.depth = 0;
          }
          t = min(t, isects.front().intersectionT);
        }
      });
  });
  eastl_size_t sz = intersected_entities.size();
  if (sz > 1)
  {
    if (do_sort == SortIntersections::YES)
      stlsort::sort_branchless(intersected_entities.begin(), intersected_entities.end());
    fill_intersections_depth(intersected_entities);
  }
  return sz != 0;
}

bool trace_entities_in_grid(uint32_t grid_hash, const Point3 &from, const Point3 &dir, float &t, ecs::EntityId ignore_eid)
{
  bool isHit = false;
  const float initialT = t;
  for_each_entity_in_grid(grid_hash, from, dir, t, 0.f /* radius */, GridEntCheck::BOUNDING, [&](ecs::EntityId eid, vec3f) {
    if (eid == ignore_eid)
      return;
    trace_grid_object_eid_ecs_query(eid,
      [&](const TMatrix &transform, const CollisionResource &collres, const AnimV20::AnimcharBaseComponent &animchar) {
        float traceT = initialT;
        isHit |= collres.traceRay(transform, &animchar.getNodeTree(), from, dir, traceT, nullptr /*norm*/);
        t = min(t, traceT);
      });
  });
  return isHit;
}

bool trace_entities_in_grid_by_capsule(uint32_t grid_hash,
  const Point3 &from,
  const Point3 &dir,
  float &t,
  float radius,
  ecs::EntityId ignore_eid,
  IntersectedEntities &intersected_entities,
  SortIntersections do_sort)
{
  const float initialT = t;
  for_each_entity_in_grid(grid_hash, from, dir, t, radius, GridEntCheck::BOUNDING, [&](ecs::EntityId eid, vec3f) {
    if (eid == ignore_eid)
      return;
    trace_grid_object_by_capsule_eid_ecs_query(eid,
      [&](const TMatrix &transform, const CollisionResource &collres, const AnimV20::AnimcharBaseComponent &animchar) {
        IntersectedNode isect;
        bool hit = collres.traceCapsule(transform, &animchar.getNodeTree(), from, dir, initialT, radius, isect);
        if (hit)
        {
          IntersectedEntity &ent = *(IntersectedEntity *)intersected_entities.push_back_uninitialized();
          ent.eid = eid;
          ent.t = isect.intersectionT;
          ent.collNodeId = isect.collisionNodeId;
          ent.norm = isect.normal;
          ent.pos = isect.intersectionPos;
          ent.depth = 0;
          t = min(t, isect.intersectionT);
        }
      });
  });
  eastl_size_t sz = intersected_entities.size();
  if (sz > 1)
  {
    if (do_sort == SortIntersections::YES)
      stlsort::sort_branchless(intersected_entities.begin(), intersected_entities.end());
    fill_intersections_depth(intersected_entities);
  }
  return sz != 0;
}

bool trace_entities_in_grid_by_capsule(uint32_t grid_hash,
  const Point3 &from,
  const Point3 &dir,
  float &t,
  float max_radius,
  ecs::EntityId ignore_eid,
  IntersectedEntities &intersected_entities,
  SortIntersections do_sort,
  const get_capsule_radius_cb_t &get_radius)
{
  const float initialT = t;
  for_each_entity_in_grid(grid_hash, from, dir, t, max_radius, GridEntCheck::BOUNDING, [&](ecs::EntityId eid, vec3f) {
    if (eid == ignore_eid)
      return;
    trace_grid_object_by_capsule_eid_ecs_query(eid,
      [&](const TMatrix &transform, const CollisionResource &collres, const AnimV20::AnimcharBaseComponent &animchar) {
        float radius = get_radius(eid);
        CollResIntersectionsType isects;
        bool hit;
        if (radius > 0.f)
        {
          isects.resize(1);
          hit = collres.traceCapsule(transform, &animchar.getNodeTree(), from, dir, initialT, radius, isects.front());
        }
        else
          hit = collres.traceRay(transform, &animchar.getNodeTree(), from, dir, initialT, isects, true /*sort*/);
        if (hit)
        {
          for (auto &isect : isects)
          {
            IntersectedEntity &ent = *(IntersectedEntity *)intersected_entities.push_back_uninitialized();
            ent.eid = eid;
            ent.t = isect.intersectionT;
            ent.collNodeId = isect.collisionNodeId;
            ent.norm = isect.normal;
            ent.pos = isect.intersectionPos;
            ent.depth = 0;
          }
          t = min(t, isects.front().intersectionT);
        }
      });
  });
  eastl_size_t sz = intersected_entities.size();
  if (sz > 1)
  {
    if (do_sort == SortIntersections::YES)
      stlsort::sort_branchless(intersected_entities.begin(), intersected_entities.end());
    fill_intersections_depth(intersected_entities);
  }
  return sz != 0;
}

bool trace_entities_in_grid_by_capsule(
  uint32_t grid_hash, const Point3 &from, const Point3 &dir, float &t, float radius, ecs::EntityId ignore_eid)
{
  const float initialT = t;
  for_each_entity_in_grid(grid_hash, from, dir, t, radius, GridEntCheck::BOUNDING, [&](ecs::EntityId eid, vec3f) {
    if (eid == ignore_eid)
      return;
    trace_grid_object_by_capsule_eid_ecs_query(eid,
      [&](const TMatrix &transform, const CollisionResource &collres, const AnimV20::AnimcharBaseComponent &animchar) {
        IntersectedNode isect;
        bool hit = collres.traceCapsule(transform, &animchar.getNodeTree(), from, dir, initialT, radius, isect);
        if (hit)
          t = min(t, isect.intersectionT);
      });
  });
  return t != initialT;
}

bool rayhit_entities_in_grid(uint32_t grid_hash, const Point3 &from, const Point3 &dir, float t, ecs::EntityId ignore_eid)
{
  return !!find_entity_in_grid(grid_hash, from, dir, t, 0.f /* radius */, GridEntCheck::BOUNDING, [&](ecs::EntityId eid, vec3f) {
    if (eid == ignore_eid)
      return false;
    bool hit = false;
    trace_grid_object_eid_ecs_query(eid,
      [&](const TMatrix &transform, const CollisionResource &collres, const AnimV20::AnimcharBaseComponent &animchar) {
        hit = collres.rayHit(transform, &animchar.getNodeTree(), from, dir, t);
      });
    return hit;
  });
}

bool query_entities_intersections_in_grid(uint32_t grid_hash,
  dag::ConstSpan<plane3f> convex,
  const TMatrix &tm,
  float rad,
  bool rayhit,
  IntersectedEntities &entities,
  SortIntersections do_sort)
{
  for_each_entity_in_grid(grid_hash, BSphere3(tm.getcol(3), rad), GridEntCheck::BOUNDING, [&](ecs::EntityId eid, vec3f) {
    intersected_eid_ecs_query(eid,
      [&](const TMatrix &transform, const CollisionResource &collres, const AnimV20::AnimcharBaseComponent &animchar) {
        dag::ConstSpan<CollisionNode> allNodes = collres.getAllNodes();
        IntersectedEntity bestNode;
        bestNode.t = FLT_MAX;
        for (int i = 0; i < allNodes.size(); ++i)
        {
          if (allNodes[i].type != COLLISION_NODE_TYPE_MESH && allNodes[i].type != COLLISION_NODE_TYPE_BOX &&
              allNodes[i].type != COLLISION_NODE_TYPE_SPHERE)
            continue;
          Point3 resPos = tm.getcol(3);
          if (!collres.testInclusion(allNodes[i], transform, convex, tm, &animchar.getNodeTree(), &resPos))
            continue;
          Point3 dir = resPos - tm.getcol(3);
          float len = length(dir);
          dir *= safeinv(len);
          if (len >= bestNode.t)
            continue;
          if (rayhit && dacoll::rayhit_normalized(tm.getcol(3), dir, len))
            continue;
          bestNode.eid = eid;
          bestNode.pos = resPos;
          bestNode.norm = -dir;
          bestNode.t = len;
          bestNode.collNodeId = i;
          bestNode.depth = 0;
        }
        if (bestNode.eid != ecs::INVALID_ENTITY_ID)
        {
          IntersectedEntity &ent = *(IntersectedEntity *)entities.push_back_uninitialized();
          ent.eid = bestNode.eid;
          ent.pos = bestNode.pos;
          ent.norm = bestNode.norm;
          ent.t = bestNode.t;
          ent.collNodeId = bestNode.collNodeId;
          ent.depth = bestNode.depth;
        }
      });
    return false;
  });
  eastl_size_t sz = entities.size();
  if (sz > 1)
  {
    if (do_sort == SortIntersections::YES)
      stlsort::sort_branchless(entities.begin(), entities.end());
    fill_intersections_depth(entities);
  }
  return sz != 0;
}
