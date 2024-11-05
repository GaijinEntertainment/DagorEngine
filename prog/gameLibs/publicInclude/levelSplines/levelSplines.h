//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ioSys/dag_genIo.h>
#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <util/dag_simpleString.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>


namespace levelsplines
{

struct SplinePoint
{
  Point3 bezIn;
  Point3 pt;
  Point3 bezOut;
};

struct Spline
{
  SimpleString name;
  SmallTab<SplinePoint, MidmemAlloc> pathPoints;
};

struct GridPoint
{
  uint16_t splineId;
  uint16_t pointId;
};

struct SplineGrid
{
  uint16_t width;
  uint16_t height;
  Point4 offsetMul;
  SmallTab<uint32_t, MidmemAlloc> grid;
  Tab<GridPoint> splinePoints;
};

bool load_splines(IGenLoad &crd, Tab<Spline> &out);
bool load_grid(IGenLoad &crd, SplineGrid &out);

IPoint2 get_spline_grid_cell(const SplineGrid &grid, const Point3 &pt);
dag::ConstSpan<GridPoint> get_spline_grid_cell_points(const SplineGrid &grid, int row, int column);

// Data about point on spline for pathfinding.
struct IntersectionSplinePoint
{
  float pos;
  uint16_t splineId;
  uint16_t _resv;
};

// For saving it on spline itself and for using it in table.
struct IntersectionNodePoint
{
  uint16_t nodeId;
  uint8_t nodePointId;
  uint8_t _resv;

  IntersectionNodePoint() : nodeId(0xffff), nodePointId(0xff), _resv(0xff) {}
  IntersectionNodePoint(uint16_t nId, uint8_t np) : nodeId(nId), nodePointId(np), _resv(0xff) {}

  bool isValid() const { return nodeId != 0xffff && nodePointId != 0xff; }

  bool operator==(const IntersectionNodePoint &rhs) const { return rhs.nodeId == nodeId && rhs.nodePointId == nodePointId; }

  bool operator!=(const IntersectionNodePoint &rhs) const { return !(*this == rhs); }
};

struct IntersectionTableRecord
{
  float dist;
  IntersectionNodePoint nextPoint;
};

struct IntersectionSplineEdge
{
  float dist;
  IntersectionNodePoint nextPoint;
  uint8_t fromPointId;
  uint8_t _resv[3];
};

struct IntersectionNode
{
  SmallTab<IntersectionSplinePoint, MidmemAlloc> points;
  SmallTab<IntersectionSplineEdge, MidmemAlloc> edges;
  Point2 position;
};

enum IntersectionDataType
{
  EIDT_TABLE,
  EIDT_EDGES
};

class SplineIntersections
{
  struct Way
  {
    Tab<IntersectionNodePoint> nodes;
    float length;
    uint16_t fromNodeId;
    uint16_t toNodeId;

    Way() : nodes(midmem) { clear(); }

    void clear()
    {
      clear_and_shrink(nodes);
      length = FLT_MAX;
      fromNodeId = 0xffff;
      toNodeId = 0xffff;
    }
  };

  static const int CACHED_WAYS_NUM = 8;

  uint16_t size;
  Tab<IntersectionNode> nodes;
  SmallTab<IntersectionTableRecord, MidmemAlloc> table;
  int intersectionDataType;

  mutable Way cachedWays[CACHED_WAYS_NUM];
  mutable int currentWay;

  Way *findWay(uint16_t fromNodeId, uint16_t toNodeId, int &nodeNum) const;

  void resize(int count);

public:
  SplineIntersections() : size(0), intersectionDataType(0), nodes(midmem), currentWay(0) {}

  void getNextIntersectionPoint(uint16_t fromNodeId, uint16_t toNodeId, IntersectionNodePoint &nextPoint) const;
  float getDist(uint16_t fromNodeId, uint16_t toNodeId) const;

  const IntersectionNode &getIntersectionNode(uint16_t nodeId) const;

  void clear();
  bool load(IGenLoad &crd);

  int getNodesCount() const { return nodes.size(); }
};

}; // namespace levelsplines
