// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "collisionTraces.h"
#include <ecs/anim/anim.h>
#include <ecs/phys/collRes.h>
#include <daECS/core/sharedComponent.h>
#include <gamePhys/collision/collisionLib.h>
#include <game/capsuleApproximation.h>

template <typename Callable>
static void entity_collres_eid_ecs_query(ecs::EntityId, Callable);
template <typename Callable>
static void capsule_approximation_eid_ecs_query(ecs::EntityId, Callable);

bool trace_to_collision_nodes(
  const Point3 &from, ecs::EntityId target, IntersectedEntities &intersected_entities, SortIntersections do_sort, float ray_tolerance)
{
  entity_collres_eid_ecs_query(target,
    [&](const CollisionResource &collres, const TMatrix &transform, const AnimV20::AnimcharBaseComponent *animchar = nullptr) {
      mat44f tm;
      v_mat44_make_from_43cu_unsafe(tm, transform.array);
      dag::ConstSpan<CollisionNode> allNodes = collres.getAllNodes();
      const auto geomTree = animchar == nullptr ? nullptr : &animchar->getNodeTree();
      for (int i = 0; i < allNodes.size(); ++i)
      {
        mat44f nodeTm;
        collres.getCollisionNodeTm(&allNodes[i], tm, geomTree, nodeTm);
        vec3f vTo = v_mat44_mul_vec3p(nodeTm, v_ldu(&allNodes[i].boundingSphere.c.x));
        vec3f vDir = v_sub(vTo, v_ldu(&from.x));
        vec4f vT = v_length3(vDir);
        vDir = v_safediv(vDir, vT);

        Point3_vec4 dir;
        v_stu_p3(&dir.x, vDir);
        float t = v_extract_x(vT);
        if (t > ray_tolerance && dacoll::rayhit_normalized(from, dir, t - max(0.f, ray_tolerance)))
          continue;

        IntersectedEntity &ent = *(IntersectedEntity *)intersected_entities.push_back_uninitialized();
        ent.eid = target;
        ent.t = t;
        ent.collNodeId = i;
        ent.norm = -dir;
        v_stu_p3(&ent.pos.x, vTo);
        ent.depth = 0;
      }
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

bool trace_to_capsule_approximation(
  const Point3 &from, ecs::EntityId target, IntersectedEntities &intersected_entities, SortIntersections do_sort, float ray_tolerance)
{
  bool isTraced = false;
  capsule_approximation_eid_ecs_query(target,
    [&](const AnimV20::AnimcharBaseComponent &animchar, ECS_SHARED(CapsuleApproximation) capsule_approximation) {
      isTraced = !capsule_approximation.capsuleDatas.empty();
      for (const auto &data : capsule_approximation.capsuleDatas)
      {
        mat44f wtm;
        animchar.getNodeTree().getNodeWtm(data.nodeIndex, wtm);

        Capsule capsule;
        capsule.set(data.a, data.b, data.rad);
        capsule.transform(wtm);

        vec3f vTo = capsule.getCenter();
        vec3f vDir = v_sub(vTo, v_ldu(&from.x));
        vec4f vT = v_length3(vDir);
        vDir = v_safediv(vDir, vT);

        Point3_vec4 dir;
        v_stu_p3(&dir.x, vDir);
        float t = v_extract_x(vT);
        if (t > ray_tolerance && dacoll::rayhit_normalized(from, dir, t - max(0.f, ray_tolerance)))
          continue;

        IntersectedEntity &ent = *(IntersectedEntity *)intersected_entities.push_back_uninitialized();
        ent.eid = target;
        ent.t = t;
        ent.collNodeId = data.collNodeId;
        ent.norm = -dir;
        v_stu_p3(&ent.pos.x, vTo);
        ent.depth = 0;
      }
    });

  if (DAGOR_UNLIKELY(!isTraced))
    return trace_to_collision_nodes(from, target, intersected_entities, do_sort, ray_tolerance);

  eastl_size_t sz = intersected_entities.size();
  if (sz > 1)
  {
    if (do_sort == SortIntersections::YES)
      stlsort::sort_branchless(intersected_entities.begin(), intersected_entities.end());
    fill_intersections_depth(intersected_entities);
  }
  return sz != 0;
}

bool rayhit_to_collision_nodes(const Point3 &from, ecs::EntityId target, float ray_tolerance)
{
  bool isHit = false;
  entity_collres_eid_ecs_query(target,
    [&](const CollisionResource &collres, const TMatrix &transform, const AnimV20::AnimcharBaseComponent *animchar) {
      mat44f tm;
      v_mat44_make_from_43cu_unsafe(tm, transform.array);
      dag::ConstSpan<CollisionNode> allNodes = collres.getAllNodes();
      const auto geomTree = animchar == nullptr ? nullptr : &animchar->getNodeTree();
      for (int i = 0; i < allNodes.size(); ++i)
      {
        mat44f nodeTm;
        collres.getCollisionNodeTm(&allNodes[i], tm, geomTree, nodeTm);
        vec3f vTo = v_mat44_mul_vec3p(nodeTm, v_ldu(&allNodes[i].boundingSphere.c.x));
        vec3f vDir = v_sub(vTo, v_ldu(&from.x));
        vec4f vT = v_length3(vDir);
        vDir = v_safediv(vDir, vT);

        Point3_vec4 dir;
        v_stu_p3(&dir.x, vDir);
        float t = v_extract_x(vT);
        if (t > ray_tolerance && dacoll::rayhit_normalized(from, dir, t - max(0.f, ray_tolerance)))
          continue;

        isHit = true;
        break;
      }
    });
  return isHit;
}

bool rayhit_to_capsule_approximation(const Point3 &from, ecs::EntityId target, float ray_tolerance)
{
  bool isTraced = false, isHit = false;
  capsule_approximation_eid_ecs_query(target,
    [&](const AnimV20::AnimcharBaseComponent &animchar, ECS_SHARED(CapsuleApproximation) capsule_approximation) {
      isTraced = true;
      for (const auto &data : capsule_approximation.capsuleDatas)
      {
        mat44f wtm;
        animchar.getNodeTree().getNodeWtm(data.nodeIndex, wtm);

        Capsule capsule;
        capsule.set(data.a, data.b, data.rad);
        capsule.transform(wtm);

        vec3f vTo = capsule.getCenter();
        vec3f vDir = v_sub(vTo, v_ldu(&from.x));
        vec4f vT = v_length3(vDir);
        vDir = v_safediv(vDir, vT);

        Point3_vec4 dir;
        v_stu_p3(&dir.x, vDir);
        float t = v_extract_x(vT);
        if (t > ray_tolerance && dacoll::rayhit_normalized(from, dir, t - max(0.f, ray_tolerance)))
          continue;

        isHit = true;
        break;
      }
    });

  if (DAGOR_UNLIKELY(!isTraced))
    return rayhit_to_collision_nodes(from, target, ray_tolerance);

  return isHit;
}
