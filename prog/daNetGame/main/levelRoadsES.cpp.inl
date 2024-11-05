// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/levelRoads.h"
#include <ecs/core/entityManager.h>
#include <daECS/core/updateStage.h>

ECS_REGISTER_RELOCATABLE_TYPE(splineroads::SplineRoads, nullptr);
ECS_AUTO_REGISTER_COMPONENT(splineroads::SplineRoads, "level_roads", nullptr, 0);

void create_level_roads(splineroads::SplineRoads &roads)
{
  G_ASSERT(!g_entity_mgr->getSingletonEntity(ECS_HASH("level_roads")));
  ecs::ComponentsInitializer attrs;
  ECS_INIT(attrs, level_roads, eastl::move(roads));
  g_entity_mgr->createEntityAsync("level_roads", eastl::move(attrs));
}

Tab<TMatrix> get_points_on_road_route(
  const Point3 &start_route_pos, const Point3 &end_route_pos, int points_count, float points_dist, float roads_search_rad)
{
  Tab<TMatrix> result(framemem_ptr());
  ecs::EntityId eid = g_entity_mgr->getSingletonEntity(ECS_HASH("level_roads"));
  if (const splineroads::SplineRoads *splineRoads = g_entity_mgr->getNullable<splineroads::SplineRoads>(eid, ECS_HASH("level_roads")))
  {
    // prepare and get first piece
    int roadId = -1;
    float srcRoadPointT = 0.f, dstRoadPointT = 0.f;
    uint16_t srcIntersectionId = 0, dstIntersectionId = 0;
    float restPointDistance = points_dist;
    if (!splineRoads->findWay(start_route_pos, end_route_pos, roads_search_rad, roadId, srcRoadPointT, dstRoadPointT,
          srcIntersectionId, dstIntersectionId))
      return result;

    // push start point
    result.push_back(splineRoads->getRoadPointMat(roadId, srcRoadPointT));

    bool isContinue = true;
    while (isContinue)
    {
      float currentPointT = srcRoadPointT;
      if (currentPointT <= dstRoadPointT)
      {
        while (currentPointT < dstRoadPointT)
        {
          currentPointT += restPointDistance;
          if (currentPointT < dstRoadPointT)
          {
            result.push_back(splineRoads->getRoadPointMat(roadId, currentPointT));
            if (result.size() == points_count)
              break;
            restPointDistance = points_dist;
          }
          else
            restPointDistance -= dstRoadPointT - (currentPointT - restPointDistance);
        }
      }
      else
      {
        while (currentPointT > dstRoadPointT)
        {
          currentPointT -= restPointDistance;
          if (currentPointT > dstRoadPointT)
          {
            result.push_back(splineRoads->getRoadPointMat(roadId, currentPointT));
            if (result.size() == points_count)
              break;
            restPointDistance = points_dist;
          }
          else
            restPointDistance -= (currentPointT + restPointDistance) - dstRoadPointT;
        }
      }

      if (result.size() < points_count && srcIntersectionId < UINT16_MAX && dstIntersectionId < UINT16_MAX // valid id
          && srcIntersectionId != dstIntersectionId)
        splineRoads->nextPointToDest(srcIntersectionId, dstIntersectionId, roadId, srcRoadPointT, dstRoadPointT);
      else
        isContinue = false;
    }
  }
  return result;
}