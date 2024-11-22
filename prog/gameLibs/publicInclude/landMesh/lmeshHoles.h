//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_TMatrix.h>
#include <math/dag_bounds2.h>
#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <generic/dag_tab.h>
#include <EASTL/bitvector.h>

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
  uint32_t count() const { return holes.size(); }
  bool empty() const { return holes.empty(); }

private:
  dag::Vector<HoleData> holes;
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

  LandMeshHolesManager(const HeightmapHandler &hmap_handler, int cells_count = 16);
  void clearAndAddHoles(const Tab<HoleArgs> &holes);
  void clearHoles();
  bool check(const Point2 &posXZ) const;

private:
  uint32_t getCellIndex(const Point2 &point) const;

  uint16_t holeCellsCount;
  bool emptyHolesRegion = true;
  BBox2 holesRegion;  // Undefined if `emptyHolesRegion` is true
  Point2 cellSizeInv; // Ditto
  dag::Vector<LandMeshHolesCell> cells;
  eastl::bitvector<> cellsNeedValidHeight;
  const HeightmapHandler &hmapHandler;
};

inline uint32_t LandMeshHolesManager::getCellIndex(const Point2 &point) const
{
  G_FAST_ASSERT(!emptyHolesRegion);
  // TODO: SIMDify this
  Point2 pOfs = (point - holesRegion[0]);
  Point2 cellPos = mul(pOfs, cellSizeInv);
  IPoint2 iCellPos = min(IPoint2(int(cellPos.x), int(cellPos.y)), IPoint2(holeCellsCount - 1, holeCellsCount - 1));
  return iCellPos.y * holeCellsCount + iCellPos.x;
}

inline bool LandMeshHolesManager::check(const Point2 &posXZ) const
{
  if (emptyHolesRegion || !(holesRegion & posXZ))
    return false;
  uint32_t ci = getCellIndex(posXZ);
  auto &cell = cells[ci];
  return !cell.empty() && cell.check(posXZ, cellsNeedValidHeight.test(ci, false) ? &hmapHandler : nullptr);
}
