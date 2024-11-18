//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_bounds2.h>
#include <math/dag_Point2.h>
#include <EASTL/vector.h>
#include <osApiWrappers/dag_rwLock.h>

class HeightmapHandler;
class LandMeshHolesCell
{
public:
  struct HoleData
  {
    bool shapeIntersection;
    bool round;
    union
    {
      Matrix3 inverseProjTm;
      TMatrix inverseTm;
    };
    HoleData(const TMatrix &tm, bool is_round, bool shape_intersection);
  };
  void addHole(const HoleData &hole);
  void clear();
  bool check(const Point2 &posXZ, const HeightmapHandler *hmapHandler) const;
  bool check(const Point3 &posXYZ) const;
  size_t count() const { return holes.size(); }

private:
  eastl::vector<HoleData> holes;
  bool needsValidHeight = false;
};
class LandMeshHolesManager
{
public:
  struct HoleArgs
  {
    bool shapeIntersection;
    bool round;
    TMatrix tm;

    HoleArgs(const TMatrix &tm, bool is_round, bool shape_intersection);
    BBox2 getBBox2() const;
  };

  LandMeshHolesManager(const HeightmapHandler *hmap_handler, int cells_count = 16);
  void clearAndAddHoles(const eastl::vector<HoleArgs> &holes);
  void clearHoles();
  bool check(const Point2 &posXZ) const;
  bool check(const Point2 &posXZ, float hmap_height) const;

private:
  size_t getCellIndex(const Point2 &p) const;
  void clearHolesInternal();

  int holeCellsCount;
  BBox2 holesRegion;
  Point2 cellSize;
  Point2 cellSizeInv;
  eastl::vector<LandMeshHolesCell> cells;
  const HeightmapHandler *hmapHandler;
  mutable SmartReadWriteFifoLock rwLock;
};
