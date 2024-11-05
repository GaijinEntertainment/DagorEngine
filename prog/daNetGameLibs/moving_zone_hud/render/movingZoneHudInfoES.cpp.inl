// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <math/dag_rayIntersectSphere.h>
#include <math/dag_mathAng.h>
#include <math/dag_mathUtils.h>
#include <3d/dag_render.h>
#include "game/player.h"


ECS_AFTER(moving_zone)
ECS_REQUIRE(ecs::Tag moving_zone_hud)
static __forceinline void moving_zone_hud_es(const ecs::UpdateStageInfoAct &info,
  const TMatrix &transform,
  const Point2 &moving_zone__startEndTime,
  Point3 moving_zone__sourcePos,
  Point3 moving_zone__targetPos,
  float moving_zone__sourceRadius,
  float moving_zone__targetRadius,
  int &moving_zone_hud__zoneStartTimer,
  int &moving_zone_hud__zoneFinishTimer,
  int &moving_zone_hud__zoneProgress,
  int &moving_zone_hud__playerZonePosition)
{
  const Point2 &startEnd = moving_zone__startEndTime;
  moving_zone_hud__zoneStartTimer = info.curTime < startEnd[0] ? startEnd[0] - info.curTime : -1.f;
  moving_zone_hud__zoneFinishTimer =
    info.curTime >= startEnd[0] && info.curTime < startEnd[1] ? max(startEnd[1] - info.curTime, 0.f) : -1.f;

  moving_zone_hud__zoneProgress =
    info.curTime >= startEnd[0] && info.curTime < startEnd[1] ? cvt(info.curTime, startEnd[0], startEnd[1], 0.f, 100.f) : -1.f;

  // playerZonePosition

  const TMatrix &heroTrans = g_entity_mgr->getOr(game::get_controlled_hero(), ECS_HASH("transform"), TMatrix::IDENT);
  const Point3 &heroPos = heroTrans.getcol(3);

  Point3 zoneDirVec = moving_zone__targetPos - heroPos;
  float distanceToZoneCenter = length(zoneDirVec);

  float distToTargetZone = distanceToZoneCenter - moving_zone__targetRadius;
  bool isInTargetZone = distToTargetZone < 0.f;
  if (!isInTargetZone)
  {
    float dist =
      rayIntersectSphereDist(heroPos, normalize(heroPos - transform.getcol(3)), moving_zone__sourcePos, moving_zone__sourceRadius);
    if (dist >= 0.f)
    {
      float sumDist = dist + distToTargetZone;
      float progressOutside = safediv(dist, sumDist);
      moving_zone_hud__playerZonePosition = progressOutside * 100.f;
    }
    else
    {
      moving_zone_hud__playerZonePosition = 0;
    }
  }
  else
    moving_zone_hud__playerZonePosition = 100;
}
