// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <heightmap/heightmapHandler.h>
#include <math/dag_bounds2.h>
#include <math/dag_mathUtils.h>

HMapTesselationData::HMapTesselationData(HeightmapHandler &in_handler) : handler(in_handler), tessCellSize(0) {}

void HMapTesselationData::init(int buffer_size, float tess_cell_size)
{
  G_ASSERT(buffer_size > 0);
  bufferSize = max(buffer_size, 1);
  tessCellSize = tess_cell_size;

  BBox3 hmapBox = handler.getWorldBox();
  pivot = Point2::xz(hmapBox.center());
  worldSize = Point2::xz(hmapBox.width());

  patchNum = handler.hmapWidth / bufferSize;
  patches.resize(patchNum.x * patchNum.y);
}

void HMapTesselationData::addTessSphere(const Point2 &pos, float radius)
{
  Point2 radiusVec = Point2(radius, radius);
  const IPoint2 hmapPos = getPatchXYFromCoord(getCoordFromPos(pos));
  const IPoint2 hmapStart = getPatchXYFromCoord(getCoordFromPos(pos - radiusVec));
  const IPoint2 hmapEnd = getPatchXYFromCoord(getCoordFromPos(pos + radiusVec)) + IPoint2(1, 1);
  const int hmapRadius = hmapEnd.x - hmapPos.x;
  const int hmapRadiusSq = hmapRadius * hmapRadius;
  const IPoint2 coordMin = IPoint2::ZERO;
  const IPoint2 coordMax = patchNum - IPoint2::ONE;
  const IPoint2 coordStart = clamp(hmapStart, coordMin, coordMax);
  const IPoint2 coordEnd = clamp(hmapEnd, coordMin, coordMax);

  for (int y = coordStart.y; y < coordEnd.y; ++y)
    for (int x = coordStart.x; x < coordEnd.x; ++x)
    {
      IPoint2 hmapCoord(x, y);
      int distSq = lengthSq(hmapPos - hmapCoord);
      if (distSq > hmapRadiusSq)
        continue;
      patches.set(getPatchNoFromXY(hmapCoord), true);
      patchHasTess = true;
    }
}

void HMapTesselationData::addTessRect(const TMatrix &transform, bool project_volume)
{
  // Y row is deleted because we project at XZ plane after transform.
  Point3 xCol = Point3(transform.col[0][0], transform.col[0][2], 0.f);
  Point3 yCol = Point3(transform.col[1][0], transform.col[1][2], 0.f);
  Point3 zCol = Point3(transform.col[2][0], transform.col[2][2], 0.f);
  Point3 centerCol = Point3(transform.col[3][0], transform.col[3][2], 1.f);
  if (!project_volume)
  {
    Matrix3 transform33;
    transform33.col[0] = xCol;
    transform33.col[1] = zCol;
    transform33.col[2] = centerCol;
    addTessRectImpl(transform33);
  }
  else
  {
    // Projecting 6 sides of parallelepiped
    Matrix3 transform33;

    // for X = +- 1
    transform33.col[0] = yCol;
    transform33.col[1] = zCol;
    transform33.col[2] = centerCol + xCol;
    addTessRectImpl(transform33);
    transform33.col[2] = centerCol - xCol;
    addTessRectImpl(transform33);

    // for Y = +- 1
    transform33.col[0] = xCol;
    transform33.col[1] = zCol;
    transform33.col[2] = centerCol + yCol;
    addTessRectImpl(transform33);
    transform33.col[2] = centerCol - yCol;
    addTessRectImpl(transform33);

    // for Z = +- 1
    transform33.col[0] = xCol;
    transform33.col[1] = yCol;
    transform33.col[2] = centerCol + zCol;
    addTessRectImpl(transform33);
    transform33.col[2] = centerCol - zCol;
    addTessRectImpl(transform33);
  }
}

void HMapTesselationData::addTessRectImpl(const Matrix3 &transform33)
{
  if (abs(transform33.det()) < FLT_EPSILON)
    return; // then rectangle is a line.

  Matrix3 inverseProjTm = inverse(transform33);

  Point2 center = Point2(transform33.getcol(2).x, transform33.getcol(2).y);
  Point2 cornersOfs =
    Point2(abs(transform33.getcol(0).x) + abs(transform33.getcol(1).x), abs(transform33.getcol(0).y) + abs(transform33.getcol(1).y));
  BBox2 hmapBox(center - cornersOfs, center + cornersOfs);

  const IPoint2 hmapStart = getPatchXYFromCoord(getCoordFromPos(hmapBox.lim[0]));
  const IPoint2 hmapEnd = getPatchXYFromCoord(getCoordFromPos(hmapBox.lim[1])) + IPoint2(1, 1);
  const IPoint2 coordMin = IPoint2::ZERO;
  const IPoint2 coordMax = patchNum - IPoint2::ONE;
  const IPoint2 coordStart = clamp(hmapStart, coordMin, coordMax);
  const IPoint2 coordEnd = clamp(hmapEnd, coordMin, coordMax);
  Point2 cellSize =
    Point2((float)bufferSize / handler.hmapWidth.x * worldSize.x, (float)bufferSize / handler.hmapWidth.y * worldSize.y);
  Point2 points[3] = {Point2(cellSize.x, 0), Point2(0, cellSize.y), cellSize};
  Point2 minInversed(0, 0), maxInversed(0, 0); // From Point2(0, 0)
  for (int i = 0; i < 3; i++)
  {
    Point3 inversedVec = inverseProjTm * Point3(points[i].x, points[i].y, 0.f);
    Point2 inversed = Point2(inversedVec.x, inversedVec.y);
    minInversed = min(minInversed, inversed);
    maxInversed = max(maxInversed, inversed);
  }

  for (int y = coordStart.y; y < coordEnd.y; ++y)
    for (int x = coordStart.x; x < coordEnd.x; ++x)
    {
      IPoint2 hmapCoord(x, y);
      Point2 patchPos = getPosFromCoord(getCoordFromPatchXY(Point2(x, y)));
      Point3 patchPosInversedP3 = inverseProjTm * Point3(patchPos.x, patchPos.y, 1.f);
      Point2 patchPosInversed = Point2(patchPosInversedP3.x, patchPosInversedP3.y);
      Point2 minP = patchPosInversed + minInversed;
      Point2 maxP = patchPosInversed + maxInversed;
      if (maxP.x < -1.f || minP.x > 1.f || maxP.y < -1.f || minP.y > 1.f)
        continue;
      patches.set(getPatchNoFromXY(hmapCoord), true);
      patchHasTess = true;
    }
}

void HMapTesselationData::tesselatePatch(const IPoint2 &cell, bool enable)
{
  IPoint2 patchXY = getPatchXYFromCoord(cell);
  int patchIndex = getPatchNoFromXY(patchXY);
  if (enable)
    patches.set(patchIndex, true);
  patchHasTess |= enable;
}

bool HMapTesselationData::testRegionTesselated(const IBBox2 &region) const
{
  if (!patchHasTess)
    return false;

  IPoint2 patchXY1 = max(getPatchXYFromCoord(region.lim[0]), IPoint2::ZERO); // todo: optimal conversion to int coord (using vector
                                                                             // intrinsics)
  IPoint2 patchXY2 = min(getPatchXYFromCoord(region.lim[1]) + IPoint2::ONE, patchNum);
  constexpr uint32_t digits = sizeof(eastl::BitvectorWordType) * CHAR_BIT;
  uint32_t w = patchXY2.x - patchXY1.x, patchNo = getPatchNoFromXY(patchXY1), patchStrideLeft = patchNum.x - w;
  auto pdata = patches.data();
  for (int y = patchXY1.y; y < patchXY2.y; ++y, patchNo += patchStrideLeft)
  {
    int x = 0;
    for (auto pbits = pdata + patchNo / digits; x < w; ++pbits)
    {
      uint32_t j = patchNo % digits;
      eastl::BitvectorWordType bits = *pbits;
      if (!bits)
      {
        uint32_t di = eastl::min(digits - j, w - x);
        x += di;
        patchNo += di;
      }
      else
      {
        for (auto tbit = eastl::BitvectorWordType(1) << j; j < digits && x < w; ++j, ++x, ++patchNo, tbit <<= 1)
          if ((bits & tbit) != 0)
            return true;
      }
    }
  }
  return false;
}

bool HMapTesselationData::testRegionTesselated(const BBox2 &region) const
{
  return testRegionTesselated(IBBox2(getCoordFromPos(region.lim[0]), getCoordFromPos(region.lim[1])));
}

bool HMapTesselationData::hasTesselation() const { return patchHasTess; }

float HMapTesselationData::getTessCellSize() const { return tessCellSize; }

IPoint2 HMapTesselationData::getPatchXYFromCoord(const IPoint2 &coord) const
{
  return IPoint2(coord.x / bufferSize, coord.y / bufferSize);
}

Point2 HMapTesselationData::getCoordFromPatchXY(const Point2 &patch_xy) const
{
  return Point2(patch_xy.x * bufferSize, patch_xy.y * bufferSize);
}

int HMapTesselationData::getPatchNoFromXY(const IPoint2 &patch_xy) const { return patch_xy.y * patchNum.x + patch_xy.x; }

Point2 HMapTesselationData::getCoordFromPosF(const Point2 &pos) const
{
  return Point2((safediv(pos.x - pivot.x, worldSize.x) + 0.5f) * handler.hmapWidth.x,
    (safediv(pos.y - pivot.y, worldSize.y) + 0.5f) * handler.hmapWidth.y);
}

IPoint2 HMapTesselationData::getCoordFromPos(const Point2 &pos) const { return IPoint2::xy(getCoordFromPosF(pos)); }

Point2 HMapTesselationData::getPosFromCoord(const Point2 &coord) const
{
  return Point2((coord.x / handler.hmapWidth.x - 0.5) * worldSize.x + pivot.x,
    (coord.y / handler.hmapWidth.y - 0.5) * worldSize.y + pivot.y);
}
