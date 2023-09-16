#include <landMesh/lmeshHoles.h>
#include <heightmap/heightmapHandler.h>

LandMeshHolesManager::LandMeshHolesCell::HoleData::HoleData(const TMatrix &tm, bool is_round, bool shape_intesection) :
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

bool LandMeshHolesManager::LandMeshHolesCell::check(const Point2 &position, const HeightmapHandler *hmapHandler) const
{
  float height = MIN_REAL;
  for (const auto &hole : holes)
  {
    if (!hole.shapeIntersection)
    {
      Point3 relativePos = hole.inverseProjTm * Point3(position.x, position.y, 1.f);
      Point2 lenNorm = abs(Point2(relativePos.x, relativePos.y));
      if (lenNorm.x > 1 || lenNorm.y > 1)
        continue;
      if (hole.round && (dot(lenNorm, lenNorm) > 1))
        continue;
    }
    else
    {
      // Get height lazily. Note: `getPosOfNearestTexel` has moved sampling position
      if (height == MIN_REAL && !hmapHandler->getHeight(position, height, nullptr))
        continue; // Then, result is up to ground holes from projection.

      Point3 posXYZ = Point3(position.x, height, position.y);
      Point3 relativePos = hole.inverseTm * posXYZ;
      Point3 lenNorm = abs(relativePos);
      if (lenNorm.x > 1 || lenNorm.y > 1 || lenNorm.z > 1)
        continue;
      if (hole.round && (dot(lenNorm, lenNorm) > 1))
        continue;
    }
    return true;
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

LandMeshHolesManager::LandMeshHolesManager(const HeightmapHandler *hmap_handler, int cells_count)
{
  holeCellsCount = cells_count;
  cells.resize(holeCellsCount * holeCellsCount);
  hmapHandler = hmap_handler;

  clearHoles();
}

void LandMeshHolesManager::clearAndAddHoles(const eastl::vector<HoleArgs> &holes)
{
  clearHoles();
  if (holes.empty())
    return;

  for (const auto &hole : holes)
    holesRegion += hole.getBBox2();

  cellSize = holesRegion.size() / holeCellsCount;
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
          cells[i * holeCellsCount + j].holes.push_back(holeData);
      }
  }
}
void LandMeshHolesManager::clearHoles()
{
  for (int i = 0; i < holeCellsCount * holeCellsCount; i++)
    cells[i].holes.clear();

  holesRegion.setempty();
  cellSize = Point2(0, 0);
  cellSizeInv = Point2(0, 0);
}
bool LandMeshHolesManager::check(const Point2 &point) const
{
  if (!hmapHandler || !(holesRegion & point))
    return false;
  Point2 pOfs = (point - holesRegion[0]);
  Point2 cellPos = Point2(pOfs.x * cellSizeInv.x, pOfs.y * cellSizeInv.y);
  int y = min(int(cellPos.y), holeCellsCount - 1);
  int x = min(int(cellPos.x), holeCellsCount - 1);
  return cells[y * holeCellsCount + x].check(point, hmapHandler);
}