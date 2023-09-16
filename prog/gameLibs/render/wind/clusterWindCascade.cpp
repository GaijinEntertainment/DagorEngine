#include <render/wind/clusterWindCascade.h>
#include <render/toroidal_update_regions.h>
#include <memory/dag_framemem.h>

void ClusterWindCascade::initGrid(const ClusterWindCascade::Desc &desc)
{
  clear();
  boxWidthNum = sqrt(desc.boxNum);
  size = desc.size;
  boxSize = safediv(size, boxWidthNum);
  position = desc.pos;
  float oneSideBoxNum = (float)boxWidthNum / 2.0f;
  position = Point3(desc.pos.x, desc.pos.y, desc.pos.z) - Point3(oneSideBoxNum * boxSize, 0.0f, oneSideBoxNum * boxSize);
  gridBoundary = BBox2(Point2(position.x, position.z), size);
  maxClusterPerBox = desc.maxClusterPerBox;
  gridBoxes.resize(desc.boxNum);
  for (int i = 0; i < desc.boxNum; ++i)
    gridBoxes[i].allIds = NO_INDEX_ALL;
  offset = IPoint2(0, 0);
  torHelper.curOrigin = IPoint2(-1000000, 100000);
  torHelper.texSize = size;
}

void ClusterWindCascade::clear() { clear_and_shrink(gridBoxes); }

ToroidalQuadRegion ClusterWindCascade::updateGridPosition(const Point3 &pos)
{
  ToroidalGatherCallback::RegionTab regions;

  position.y = pos.y;

  Point2 alignedOrigin = Point2::xz(pos);
  float texelSize = 1.0f; // once we use texture, use real texelsize
  IPoint2 newTexelsOrigin = (ipoint2((alignedOrigin / (int)(texelSize))));

  IPoint2 move = abs(torHelper.curOrigin - newTexelsOrigin);
  if (move.x > (int)boxSize || move.y > (int)boxSize)
  {
    const float fullUpdateThreshold = 0.5;
    const int fullUpdateThresholdTexels = fullUpdateThreshold * torHelper.texSize;
    if (move.x < move.y)
      newTexelsOrigin.x = torHelper.curOrigin.x;
    else
      newTexelsOrigin.y = torHelper.curOrigin.y;
    ToroidalGatherCallback cb(regions);
    toroidal_update(newTexelsOrigin, torHelper, fullUpdateThresholdTexels, cb);
    int maxSideBox = 0;
    int index = -1;
    for (int i = 0; i < regions.size(); ++i)
    {
      if (regions[i].wd.x > regions[i].wd.y)
      {
        if ((regions[i].wd.x * texelSize) > maxSideBox)
        {
          maxSideBox = (regions[i].wd.x * texelSize);
          index = i;
        }
      }
      else
      {
        if ((regions[i].wd.y * texelSize) > maxSideBox)
        {
          maxSideBox = (regions[i].wd.y * texelSize);
          index = i;
        }
      }
    }
    if (index >= 0)
      return regions[index];
  }
  return ToroidalQuadRegion(IPoint2(0, 0), IPoint2(0, 0), IPoint2(0, 0));
}

bool ClusterWindCascade::isSphereOnBox(const BBox2 &box, const Point2 &pos, float r) const
{
  float dmin = 0;
  if (pos.x < box.getMin().x)
    dmin += SQR(pos.x - box.getMin().x);
  if (pos.x > box.getMax().x)
    dmin += SQR(pos.x - box.getMax().x);

  if (pos.y < box.getMin().y)
    dmin += SQR(pos.y - box.getMin().y);
  if (pos.y > box.getMax().y)
    dmin += SQR(pos.y - box.getMax().y);

  float sqr = SQR(r);
  bool value = dmin <= sqr;
  return value;
}

void ClusterWindCascade::updateOffset(const IPoint2 &offsetToAdd)
{
  offset += offsetToAdd;
  position += Point3((float)offsetToAdd.x * boxSize, 0.0f, (float)offsetToAdd.y * boxSize);
  // recalculate gridBoundaries
  float oneSideBoxNum = (float)boxWidthNum / 2.0f;
  Point2 centeredPosition = Point2(position.x, position.z) - Point2(oneSideBoxNum * boxSize, oneSideBoxNum * boxSize);
  gridBoundary.makebox(centeredPosition, boxSize * boxWidthNum);
}

void ClusterWindCascade::setOffset(const IPoint2 &newOffset)
{
  offset = newOffset;
  position += Point3((float)newOffset.x * boxSize, 0.0f, (float)newOffset.y * boxSize);
  // recalculate gridBoundaries
  float oneSideBoxNum = (float)boxWidthNum / 2.0f;
  Point2 centeredPosition = Point2(position.x, position.z) - Point2(oneSideBoxNum * boxSize, oneSideBoxNum * boxSize);
  gridBoundary.makebox(centeredPosition, boxSize * boxWidthNum);
}

void ClusterWindCascade::removeClusterFromGrids(int clusterId)
{
  for (int i = 0; i < gridBoxes.size(); ++i)
    for (int j = 0; j < maxClusterPerBox; ++j)
      if (gridBoxes[i].id[j] == clusterId)
      {
        for (int w = j; w < maxClusterPerBox - 1; ++w)
          gridBoxes[i].id[w] = gridBoxes[i].id[w + 1];
        gridBoxes[i].id[maxClusterPerBox - 1] = NO_INDEX;
      }

  // reduce all indexs
  for (int i = 0; i < gridBoxes.size(); ++i)
  {
    for (int j = 0; j < maxClusterPerBox - 1; ++j)
      if (gridBoxes[i].id[j] != NO_INDEX)
      {
        if (gridBoxes[i].id[j] > clusterId)
          gridBoxes[i].id[j] = gridBoxes[i].id[j] - 1;
      }
    gridBoxes[i].id[maxClusterPerBox - 1] = NO_INDEX;
  }
}

bool ClusterWindCascade::isClusterOnBox(int boxId, Point2 pos, float r)
{
  Point3 boxPos = getBoxPosition(boxId);
  BBox2 box(Point2(boxPos.x, boxPos.z), boxSize);
  return isSphereOnBox(box, pos, r);
}

bool ClusterWindCascade::isClusterOnCascade(const Point3 &pos, float r) const
{
  Point2 position2D = Point2(pos.x, pos.z);
  return isSphereOnBox(gridBoundary, position2D, r);
}

const int ClusterWindCascade::getAllClusterId(int boxId) const
{
  int y = boxId / boxWidthNum;
  int x = boxId - y * boxWidthNum;
  y = abs((y + offset.y) % boxWidthNum);
  x = abs((x + offset.x) % boxWidthNum);
  int boxIdWithOff = (y * boxWidthNum) + x;
  return gridBoxes[boxIdWithOff].allIds;
}

int ClusterWindCascade::getClusterId(int boxId, int at)
{
  int y = boxId / boxWidthNum;
  int x = boxId - y * boxWidthNum;
  y = abs((y + offset.y) % boxWidthNum);
  x = abs((x + offset.x) % boxWidthNum);
  int boxIdWithOff = (y * boxWidthNum) + x;
  return gridBoxes[boxIdWithOff].id[at];
}

int ClusterWindCascade::getClosestStaticBoxId(const Point3 &pos) const
{
  Point3 actualPos = getActualPosition();
  Point2 gridToCluster = Point2(actualPos.x, actualPos.z) - Point2(pos.x, pos.z);
  int x = gridToCluster.x / boxSize;
  x = x < 0 ? 0 : x;
  x = x >= boxWidthNum ? boxWidthNum - 1 : x;
  int y = gridToCluster.y / boxSize;
  y = y < 0 ? 0 : y;
  y = y >= boxWidthNum ? boxWidthNum - 1 : y;
  return y * boxWidthNum + x;
}

// cascade X/Y without toroidal wrapping offset used for world pos
IPoint2 ClusterWindCascade::getStaticBoxXY(int boxId)
{
  IPoint2 toReturn;
  toReturn.y = boxId / boxWidthNum;
  toReturn.x = boxId - toReturn.y * boxWidthNum;
  return toReturn;
}
// cascade X/Y with offset used for index search
IPoint2 ClusterWindCascade::getBoxXY(int boxId)
{
  IPoint2 toReturn;
  toReturn.y = boxId / boxWidthNum + offset.y;
  toReturn.x = boxId - toReturn.y * boxWidthNum + offset.x;
  return toReturn;
}

void ClusterWindCascade::setClusterIndexToBox(int boxId, int clusterId, int at)
{
  unsigned char tempByte = NO_INDEX;
  unsigned char secondTempByte = NO_INDEX;
  int y = boxId / boxWidthNum;
  int x = boxId - (y * boxWidthNum);
  y = abs((y + offset.y) % boxWidthNum);
  x = abs((x + offset.x) % boxWidthNum);
  int boxIdWithOffset = x + y * boxWidthNum;
  if (at + 1 < maxClusterPerBox)
    tempByte = gridBoxes[boxIdWithOffset].id[at];
  gridBoxes[boxIdWithOffset].id[at] = clusterId;
  for (int i = at + 1; i < maxClusterPerBox; ++i)
  {
    if (i + 1 < maxClusterPerBox)
      secondTempByte = gridBoxes[boxIdWithOffset].id[i];
    gridBoxes[boxIdWithOffset].id[i] = tempByte;
    tempByte = secondTempByte;
  }
}

void ClusterWindCascade::resetId(int boxId = -1)
{
  int y = boxId / boxWidthNum;
  int x = boxId - (y * boxWidthNum);
  y = abs((y + offset.y) % boxWidthNum);
  x = abs((x + offset.x) % boxWidthNum);
  int boxIdWithOffset = x + y * boxWidthNum;
  if (boxIdWithOffset >= 0 && boxIdWithOffset < SQR(boxWidthNum))
  {
    gridBoxes[boxIdWithOffset].allIds = NO_INDEX_ALL;
    return;
  }
  for (int i = 0; i < gridBoxes.size(); ++i)
    gridBoxes[i].allIds = NO_INDEX_ALL;
}

const Point3 ClusterWindCascade::getActualPosition() const { return position; }

Point3 ClusterWindCascade::getBoxPosition(int boxId)
{
  IPoint2 boxXY = getStaticBoxXY(boxId);
  return position - Point3((boxXY.x + 0.5f) * boxSize, 0.0, (boxXY.y + 0.5f) * boxSize); // centered box
}
