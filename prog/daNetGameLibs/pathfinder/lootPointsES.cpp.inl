// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <ioSys/dag_dataBlock.h>
#include <math/random/dag_random.h>
#include <math/dag_mathUtils.h>
#include <pathFinder/pathFinder.h>
#include <util/dag_simpleString.h>
#include <osApiWrappers/dag_miscApi.h>
#include <gamePhys/collision/collisionLib.h>
#include <memory/dag_framemem.h>
#include <osApiWrappers/dag_direct.h>
#include <perfMon/dag_cpuFreq.h>

#include "main/level.h"

static void do_generate_loot_points(ecs::Point3List &loot_points,
  const Point3 &loot_points__center,
  float loot_points__radius,
  int loot_points__numPoints,
  bool loot_points__underRoof,
  float loot_points__closeRange,
  float loot_points__farRange)
{
  Tab<Point3> projections(framemem_ptr());
  for (int i = 0; i < loot_points__numPoints; ++i)
  {
    Point3 pos = loot_points__center + Point3(gsrnd(), 0.f, gsrnd()) * loot_points__radius;
    projections.clear();
    pathfinder::query_navmesh_projections(pos, Point3(0.5f, FLT_MAX, 0.5f), projections);
    bool found = false;
    for (int j = 0; j < projections.size(); ++j)
    {
      float dist = 40.f;
      if (!dacoll::traceray_normalized(projections[j] + Point3(0.f, 1.f, 0.f), Point3(0.f, -1.f, 0.f), dist, NULL, NULL))
        continue;
      float ht = projections[j].y + 1.f - dist;
      dist = 40.f;
      if (loot_points__underRoof &&
          !dacoll::traceray_normalized(projections[j] + Point3(0.f, 1.f, 0.f), Point3(0.f, 1.f, 0.f), dist, NULL, NULL))
        continue;
      Point3 projPos = Point3::xVz(projections[j], ht);
      bool goodToGo = true;
      for (int k = 0; k < loot_points.size(); ++k)
      {
        float lenSq = lengthSq(Point2::xz(projPos - loot_points[k]));
        if (lenSq < sqr(loot_points__closeRange) || lenSq > sqr(loot_points__farRange))
          continue;
        goodToGo = false;
        break;
      }
      if (!goodToGo)
        continue;
      loot_points.push_back(projPos);
      found = true;
    }
    if (!found)
      i--;
  }
}

void loot_points_es_event_handler(const EventGameObjectsCreated &,
  ecs::Point3List &loot_points,
  const Point3 &loot_points__center,
  float loot_points__radius,
  int loot_points__numPoints,
  const ecs::string &loot_points__blk,
  bool loot_points__underRoof = true,
  float loot_points__closeRange = 5.f,
  float loot_points__farRange = 10.f)
{
  DataBlock blk(framemem_ptr());
  if (!loot_points__blk.empty() && dblk::load(blk, loot_points__blk.c_str(), dblk::ReadFlag::ROBUST))
  {
    for (int i = 0; i < blk.paramCount(); ++i)
      if (blk.getParamType(i) == DataBlock::TYPE_POINT3)
        loot_points.push_back(blk.getPoint3(i));
    if (loot_points.size() == loot_points__numPoints) // TODO: invalidate cache on level change
    {
      debug("loaded %d loot points from %s", loot_points.size(), loot_points__blk.c_str());
      return;
    }
    else
      loot_points.clear();
  }

  int ts = get_time_msec();
  do_generate_loot_points(loot_points, loot_points__center, loot_points__radius, loot_points__numPoints, loot_points__underRoof,
    loot_points__closeRange, loot_points__farRange);
  debug("generated %d loot points for %d ms", loot_points.size(), get_time_msec() - ts);

  // now output them to blk for further use
  if (!loot_points__blk.empty())
  {
    blk.reset();
    for (const Point3 &pt : loot_points)
      blk.addPoint3("lootPoint", pt);
    if (dblk::save_to_binary_file(blk, loot_points__blk.c_str()))
      debug("saved %d loot points to %s", loot_points.size(), loot_points__blk.c_str());
  }
}
