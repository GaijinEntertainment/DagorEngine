// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "waterFlowmapObstacles.h"
#include <ecs/core/entityManager.h>

inline void WaterFlowmapObstacles::GatherObstacleEventCtx::addCircularObstacle(const BBox3 &box, const TMatrix &tm)
{
  float radius = (box.lim[1].x - box.lim[0].x + box.lim[1].z - box.lim[0].z) * 0.25;
  if (radius <= 1e-5) // bounding box can be empty, because resource hasn't been loaded yet
    return;

  const Point3 &center = tm.getcol(3);
  Point2 position = Point2(center.x, center.z);

  waterFlowmap.circularObstacles.push_back({position, radius});
};

inline void WaterFlowmapObstacles::GatherObstacleEventCtx::addRectangularObstacle(const BBox3 &box, const TMatrix &tm)
{
  float radius = (box.lim[1].x - box.lim[0].x + box.lim[1].z - box.lim[0].z) * 0.25;
  if (radius <= 1e-5) // bounding box can be empty, because resource hasn't been loaded yet
    return;

  const Point3 &center = tm.getcol(3);
  const Point3 &axis = tm.getcol(0);
  Point2 position = Point2(center.x, center.z);
  Point2 rotation = Point2(axis.x, axis.z);
  rotation.normalize();
  Point2 size = Point2(box.lim[1].x - box.lim[0].x, box.lim[1].z - box.lim[0].z);

  waterFlowmap.rectangularObstacles.push_back({position, rotation, size});
};

ECS_BROADCAST_EVENT_TYPE(GatherObstaclesForWaterFlowmap, WaterFlowmapObstacles::GatherObstacleEventCtx *)
