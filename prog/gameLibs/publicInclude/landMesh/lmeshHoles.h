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
  void addHole(const HoleData &hole, const BBox2 &hole_bbox);
  void clear();
  bool check(const Point2 &posXZ, const HeightmapHandler *hmapHandler) const;
  bool check(const Point3 &posXYZ) const;
  uint32_t count() const { return holes.size(); }
  bool empty() const { return holes.empty(); }
  bool approximateCheckBBox(const BBox2 &bbox) { return accumulatedHoleBbox & bbox; }

private:
  dag::Vector<HoleData> holes;
  BBox2 accumulatedHoleBbox;
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
  bool approximateCheckBBox(const BBox2 &bbox);
  BBox2 getBbox() const { return emptyHolesRegion ? BBox2{} : holesRegion; }

private:
  IPoint2 getCellPosition(const Point2 &point) const;
  uint32_t getCellIndex(const Point2 &point) const;

  uint16_t holeCellsCount;
  bool emptyHolesRegion = true;
  BBox2 holesRegion;  // Undefined if `emptyHolesRegion` is true
  Point2 cellSizeInv; // Ditto
  dag::Vector<LandMeshHolesCell> cells;
  eastl::bitvector<> cellsNeedValidHeight;
  const HeightmapHandler &hmapHandler;
};

inline IPoint2 LandMeshHolesManager::getCellPosition(const Point2 &point) const
{
  G_FAST_ASSERT(!emptyHolesRegion);
  // TODO: SIMDify this
  Point2 pOfs = (point - holesRegion[0]);
  Point2 cellPos = mul(pOfs, cellSizeInv);
  IPoint2 iCellPos = min(IPoint2(int(cellPos.x), int(cellPos.y)), IPoint2(holeCellsCount - 1, holeCellsCount - 1));
  return iCellPos;
}

inline uint32_t LandMeshHolesManager::getCellIndex(const Point2 &point) const
{
  IPoint2 iCellPos = getCellPosition(point);
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

inline bool LandMeshHolesManager::approximateCheckBBox(const BBox2 &bbox)
{
  if (emptyHolesRegion || !(holesRegion & bbox))
    return false;
  IPoint2 minCell = getCellPosition(bbox.getMin());
  IPoint2 maxCell = getCellPosition(bbox.getMax());
  if (minCell.x > holeCellsCount - 1 || minCell.y > holeCellsCount - 1 || maxCell.x < 0 || maxCell.y < 0)
    return false;
  minCell = max(minCell, IPoint2(0, 0));
  maxCell = min(maxCell, IPoint2(holeCellsCount - 1, holeCellsCount - 1));
  for (int j = minCell.y; j <= maxCell.y; j++)
  {
    for (int i = minCell.x; i <= maxCell.x; i++)
    {
      auto &cell = cells[j * holeCellsCount + i];
      if (cell.approximateCheckBBox(bbox))
        return true;
    }
  }
  return false;
}
