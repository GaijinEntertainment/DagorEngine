// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <landMesh/lmeshHoles.h>
#include <heightmap/heightmapHandler.h>
#include <osApiWrappers/dag_rwLock.h>

LandMeshHolesCell::HoleData::HoleData(const TMatrix &tm, bool is_round, bool shape_intesection) :
  round(is_round), shapeIntersection(shape_intesection)
{
  if (!shapeIntersection)
  {
    Matrix3 tm33; // tm with second row and second column removed, due to XZ projection before and after tm.
    tm33.col[0] = {tm.col[0][0], tm.col[0][2], 0.f};
    tm33.col[1] = {tm.col[2][0], tm.col[2][2], 0.f};
    tm33.col[2] = {tm.col[3][0], tm.col[3][2], 1.f};
    inverseProjTm = inverse(tm33);
  }
  else
  {
    inverseTm = inverse(tm);
  }
}

void LandMeshHolesCell::addHole(const HoleData &hole, const BBox2 &hole_bbox)
{
  holes.push_back(hole);
  accumulatedHoleBbox += hole_bbox;
}

void LandMeshHolesCell::clear() { holes.clear(); }

static bool checkHoleProjection(const Matrix3 &inverse_proj_tm, bool is_round, const Point2 &pos)
{
  Point3 relativePos = inverse_proj_tm * Point3(pos.x, pos.y, 1.f);
  Point2 lenNorm = abs(Point2(relativePos.x, relativePos.y));
  if (lenNorm.x > 1 || lenNorm.y > 1)
    return false;
  if (is_round && (dot(lenNorm, lenNorm) > 1))
    return false;
  return true;
}
static bool checkHoleShapeIntersection(const TMatrix &inverse_tm, bool is_round, const Point3 &pos)
{
  Point3 relativePos = inverse_tm * pos;
  Point3 lenNorm = abs(relativePos);
  if (lenNorm.x > 1 || lenNorm.y > 1 || lenNorm.z > 1)
    return false;
  if (is_round && (dot(lenNorm, lenNorm) > 1))
    return false;
  return true;
}
bool LandMeshHolesCell::check(const Point2 &posXZ, const HeightmapHandler *hmapHandler) const
{
  float height = MIN_REAL;
  if (hmapHandler)
    hmapHandler->getHeight(posXZ, height, nullptr);
  return check(Point3::xVy(posXZ, height));
}
bool LandMeshHolesCell::check(const Point3 &posXYZ) const
{
  for (const auto &hole : holes)
  {
    if (!hole.shapeIntersection)
    {
      if (checkHoleProjection(hole.inverseProjTm, hole.round, Point2::xz(posXYZ)))
        return true;
    }
    else
    {
      if (checkHoleShapeIntersection(hole.inverseTm, hole.round, posXYZ))
        return true;
    }
  }
  return false;
}

LandMeshHolesManager::HoleArgs::HoleArgs(const TMatrix &tm, bool is_round, bool shape_intersection) :
  tm(tm), round(is_round), shapeIntersection(shape_intersection)
{}

BBox2 LandMeshHolesManager::HoleArgs::getBBox2() const
{
  Point2 center = Point2::xz(tm.getcol(3));
  Point2 cornersOfs = Point2(abs(tm.getcol(0).x) + abs(tm.getcol(2).x) + (shapeIntersection ? abs(tm.getcol(1).x) : 0.f),
    abs(tm.getcol(0).z) + abs(tm.getcol(2).z) + (shapeIntersection ? abs(tm.getcol(1).z) : 0.f));
  return BBox2(center - cornersOfs, center + cornersOfs);
}

LandMeshHolesManager::LandMeshHolesManager(const HeightmapHandler &hmap_handler, int cells_count) : hmapHandler(hmap_handler)
{
  holeCellsCount = cells_count;
  G_FAST_ASSERT(holeCellsCount == cells_count); // No overflow
  cells.resize(holeCellsCount * holeCellsCount);
}

void LandMeshHolesManager::clearAndAddHoles(const Tab<HoleArgs> &holes)
{
  clearHoles();
  if (holes.empty())
    return;

  for (const auto &hole : holes)
    holesRegion += hole.getBBox2();
  emptyHolesRegion = false;

  Point2 cellSize = holesRegion.size() / holeCellsCount;
  cellSizeInv = Point2(1 / cellSize.x, 1 / cellSize.y);

  for (const auto &hole : holes)
  {
    BBox2 holeBBox = hole.getBBox2();
    LandMeshHolesCell::HoleData holeData(hole.tm, hole.round, hole.shapeIntersection);
    Point2 lb;
    int i, j;
    for (i = 0, lb.y = holesRegion[0].y; i < holeCellsCount; i++, lb.y += cellSize.y)
      for (j = 0, lb.x = holesRegion[0].x; j < holeCellsCount; j++, lb.x += cellSize.x)
      {
        if (holeBBox & BBox2(lb, lb + cellSize))
        {
          int k = i * holeCellsCount + j;
          cells[k].addHole(holeData, holeBBox);
          if (holeData.shapeIntersection)
            cellsNeedValidHeight.set(k, true);
        }
      }
  }
}

void LandMeshHolesManager::clearHoles()
{
  for (auto &c : cells)
    c.clear();
  cellsNeedValidHeight.clear();
  emptyHolesRegion = true;
}
