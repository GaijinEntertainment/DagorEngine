// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daECS/core/entityId.h>
#include <math/dag_Point3.h>
#include <memory/dag_framemem.h>
#include <generic/dag_relocatableFixedVector.h>

enum class SortIntersections : uint8_t
{
  NO,
  YES
};

struct IntersectedEntityEid
{
  ecs::EntityId eid;
};
struct IntersectedEntityData
{
  union
  {
    float t;
    int sortKey; // Note: integer compare is faster then FP's one
  };
  int collNodeId;
  Point3 pos;
  Point3 norm;
  int depth;
};
struct IntersectedEntity : public IntersectedEntityEid, public IntersectedEntityData
{
  IntersectedEntity() = default;
  IntersectedEntity(ecs::EntityId eid, const IntersectedEntityData &data) : IntersectedEntityEid{eid}, IntersectedEntityData{data} {}
  bool operator<(const IntersectedEntity &other) const { return sortKey < other.sortKey; }
};

using IntersectedEntities = dag::RelocatableFixedVector<IntersectedEntity, 64, true, framemem_allocator>;

static inline void fill_intersections_depth(IntersectedEntities &intersected_entities)
{
  int depth = 0;
  ecs::EntityId eid = ecs::INVALID_ENTITY_ID;
  for (IntersectedEntity &ent : intersected_entities)
  {
    if (ent.eid != eid)
    {
      eid = ent.eid;
      depth = 0;
    }
    else
      depth++;
    ent.depth = depth;
  }
}

bool trace_to_collision_nodes(const Point3 &from,
  ecs::EntityId target,
  IntersectedEntities &intersected_entities,
  SortIntersections do_sort = SortIntersections::NO,
  float ray_tolerance = -1.f);
bool trace_to_capsule_approximation(const Point3 &from,
  ecs::EntityId target,
  IntersectedEntities &intersected_entities,
  SortIntersections do_sort = SortIntersections::NO,
  float ray_tolerance = -1.f);
bool rayhit_to_collision_nodes(const Point3 &from, ecs::EntityId target, float ray_tolerance = -1.f);
bool rayhit_to_capsule_approximation(const Point3 &from, ecs::EntityId target, float ray_tolerance = -1.f);
