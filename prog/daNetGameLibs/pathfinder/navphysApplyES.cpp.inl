// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <phys/physEvents.h>
#include <pathFinder/pathFinder.h>

ECS_NO_ORDER
static inline void navphys_apply_es_event_handler(const CmdNavPhysCollisionApply &evt,
  int64_t &navmesh_phys__currentPoly,
  Point3 &navmesh_phys__currentPos,
  Point3 &navmesh_phys__currentWalkVelocity)
{
  navmesh_phys__currentWalkVelocity = *evt.get<0>() /*velocity*/;
  pathfinder::FindRequest req{navmesh_phys__currentPos, navmesh_phys__currentPos + *evt.get<1>() /*pseudoVelDelta*/,
    POLYFLAGS_WALK | pathfinder::POLYFLAG_JUMP, 0, Point3(0.5f, 0.5f, 0.5f)};
  if (pathfinder::move_along_surface(req))
  {
    navmesh_phys__currentPos = req.end;
    navmesh_phys__currentPoly = req.endPoly;
  }
}
