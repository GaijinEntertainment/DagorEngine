//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_Point2.h>
#include <math/dag_polyUtils.h>
#include <levelSplines/levelSplines.h>
#include <ioSys/dag_dataBlock.h>

namespace splineregions
{
struct SplineRegion;
};
typedef Tab<splineregions::SplineRegion> LevelRegions;
namespace splineregions
{
struct RegionConvex
{
  Tab<Line2> lines;
  float area;

  bool checkPoint(const Point2 &pt) const;
  bool operator==(const RegionConvex &rc) const { return lines.size() == rc.lines.size() && mem_eq(lines, rc.lines.data()); }
};

struct SplineRegion
{
  SimpleString name;
  Tab<RegionConvex> convexes; // convexes which we use to test against any incoming 2d points
  // this is border from spline, so we can easily visualize it in any way we want,
  // as well as have it as raw source for any UI visualization
  Tab<Point3> border;
  Tab<Point3> originalBorder; // todo: debug reason, may be removed any time
  Tab<int> triangulationIndexes;
  float area;
  bool isVisible;

  // These are approximate. It's possible to improve them if needed.
  float boundingRadius;
  Point3 center;

  bool checkPoint(const Point2 &pt) const;
  Point3 getAnyBorderPoint() const;
  Point3 getRandomBorderPoint() const;
  Point3 getRandomPoint() const;
  const char *getNameStr() const;

  bool operator==(const SplineRegion &sr) const
  {
    return name == sr.name && convexes == sr.convexes && border.size() == sr.border.size() && mem_eq(border, sr.border.data()) &&
           originalBorder.size() == sr.originalBorder.size() && mem_eq(originalBorder, sr.originalBorder.data());
  }
};

void construct_region_from_spline(SplineRegion &region, const levelsplines::Spline &spline, float expand_dist = 0.f);
void load_regions_from_splines(LevelRegions &regions, dag::ConstSpan<levelsplines::Spline> splines, const DataBlock &blk);
}; // namespace splineregions
