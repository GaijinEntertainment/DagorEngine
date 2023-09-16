//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <math/dag_bezier.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <levelSplines/levelSplines.h>

namespace splineroads
{

struct RoadObject
{
  SimpleString name;
  // We are saving spline data in bezierspline struct
  BezierSpline3d spl;
  // and t in this tab
  SmallTab<float, MidmemAlloc> pointData;
  bool enabled;

  SmallTab<levelsplines::IntersectionNodePoint, MidmemAlloc> pathPoints;
};

class SplineRoads
{
  levelsplines::SplineGrid roadsGrid;
  Tab<RoadObject> roadObjects;
  levelsplines::SplineIntersections intersections;

  bool findPathPoints(int spline_id, float pos, levelsplines::IntersectionNodePoint &pt1,
    levelsplines::IntersectionNodePoint &pt2) const;

public:
  bool findWay(const Point3 &src, const Point3 &dst, float dist_around, // input
    int &spline_id, float &src_spl_point_t, float &dst_spl_point_t,     // immediate output
    uint16_t &node_id, uint16_t &dest_node_id) const;                   // path info

  void nextPointToDest(uint16_t &node_id, uint16_t dst, int &spline_id, float &src_spl_point_t, float &dst_spl_point_t) const;

  bool getRoadAround(const Point3 &src, const Point3 &dst, float dist_around, int &src_spline_id, int &dst_spline_id, float &pt_src,
    float &pt_dst, bool same_spline, bool dont_turn_around = false) const;

  Point3 getRoadPoint(int spline_id, float pt) const;
  Point3 getRoadPoint(int spline_id, int point_id) const;
  Point2 getRoadPoint2D(int spline_id, float pt) const;
  Point2 getRoadPoint2D(int spline_id, int point_id) const;
  TMatrix getRoadPointMat(int spline_id, float pt) const;
  TMatrix getRoadPointMat(int spline_id, int point_id) const;
  float getRoadPointT(int spline_id, int point_id) const;

  float getClosest(const Point2 &pt, int spline_id, int around, float &dist) const;

  bool loadRoadObjects(dag::ConstSpan<levelsplines::Spline> splines);
  bool loadRoadObjects(IGenLoad &crd);
  bool loadRoadGrid(IGenLoad &crd);
  bool loadIntersections(IGenLoad &crd);
};

}; // namespace splineroads
