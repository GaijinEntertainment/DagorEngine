// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <levelSplines/levelSplines.h>
#include <memory/dag_framemem.h>

namespace levelsplines
{

bool load_splines(IGenLoad &crd, Tab<Spline> &out)
{
  int splinesCount = crd.readInt();
  out.resize(splinesCount);
  for (int i = 0; i != splinesCount; ++i)
  {
    crd.readString(out[i].name);
    crd.readTab(out[i].pathPoints);
  }
  return true;
}

bool load_grid(IGenLoad &crd, SplineGrid &out)
{
  // Offsets section.
  out.width = crd.readInt();
  out.height = crd.readInt();
  crd.read(&out.offsetMul, sizeof(Point4));
  clear_and_resize(out.grid, out.width * out.height + 1);
  for (uint16_t i = 0; i != out.width; ++i)
    for (uint16_t j = 0; j != out.height; ++j)
      out.grid[j * out.width + i] = crd.readInt();

  // Sorted spline-point ids section.
  crd.readTab(out.splinePoints);
  out.grid.back() = out.splinePoints.size();
  return true;
}

IPoint2 get_spline_grid_cell(const SplineGrid &grid, const Point3 &pt)
{
  return IPoint2(int(clamp((pt.x - grid.offsetMul.x) * grid.offsetMul.z, 0.f, float(grid.width - 1))),
    int(clamp((pt.z - grid.offsetMul.y) * grid.offsetMul.w, 0.f, float(grid.height - 1))));
}

dag::ConstSpan<GridPoint> get_spline_grid_cell_points(const SplineGrid &grid, int row, int column)
{
  int currentPos = grid.grid[row * grid.width + column];
  int clampedTopPos = grid.grid[row * grid.width + column + 1];
  return dag::ConstSpan<GridPoint>(&grid.splinePoints[currentPos], clampedTopPos - currentPos);
}

struct PathFinderInfo
{
  IntersectionSplineEdge prevEdge;
  float distToDst;
  uint8_t prevPointId;
  bool navigated;

  PathFinderInfo() : prevEdge(), distToDst(FLT_MAX), prevPointId(0xff), navigated(false) {}
};

SplineIntersections::Way *SplineIntersections::findWay(uint16_t fromNodeId, uint16_t toNodeId, int &nodeNum) const
{
  if (fromNodeId == toNodeId)
    return nullptr; // Same node
  nodeNum = 0;
  // Check if we have cached this way already.
  for (int i = 0; i < CACHED_WAYS_NUM; ++i)
  {
    if (cachedWays[i].toNodeId != toNodeId)
      continue;
    if (cachedWays[i].fromNodeId == fromNodeId)
      return &cachedWays[i];
    for (int j = 0; j < cachedWays[i].nodes.size(); ++j)
    {
      if (cachedWays[i].nodes[j].nodeId != fromNodeId)
        continue;
      nodeNum = j + 1;
      return &cachedWays[i];
    }
  }
  Tab<PathFinderInfo> pfInfos(framemem_ptr());
  pfInfos.resize(size);

  pfInfos[fromNodeId].prevEdge.dist = 0.f;
  pfInfos[fromNodeId].prevEdge.nextPoint.nodeId = fromNodeId;
  pfInfos[fromNodeId].prevEdge.nextPoint.nodePointId = 0;
  pfInfos[fromNodeId].prevPointId = 0;
  pfInfos[fromNodeId].navigated = true;

  Point2 dstPos = nodes[toNodeId].position;
  for (int i = 0; i < size; ++i)
    pfInfos[i].distToDst = (dstPos - nodes[i].position).length();

  Tab<uint16_t> openNodes(framemem_ptr());
  openNodes.push_back(fromNodeId);
  while (!openNodes.empty())
  {
    int shortest = -1;
    float minDist = FLT_MAX;
    for (int i = 0; i < openNodes.size(); ++i)
    {
      uint16_t nodeId = openNodes[i];
      G_ASSERT(nodeId < size);
      const PathFinderInfo &pfInfo = pfInfos[nodeId];
      float dist = pfInfo.distToDst + pfInfo.prevEdge.dist;
      if (dist < minDist)
      {
        shortest = i;
        minDist = dist;
      }
    }
    uint16_t nodeId = openNodes[shortest];
    if (nodeId == toNodeId)
    {
      // We have arrived. Cache this way and return.
      Way *way = &cachedWays[currentWay];
      way->clear();
      way->fromNodeId = fromNodeId;
      way->toNodeId = toNodeId;
      way->length = pfInfos[toNodeId].prevEdge.dist;
      uint16_t curNode = toNodeId;
      while (curNode != fromNodeId)
      {
        const IntersectionSplineEdge &edge = pfInfos[curNode].prevEdge;
        // const IntersectionSplineEdge& prevEdge = pfInfos[edge.nextPoint.nodeId].prevEdge;
        IntersectionNodePoint pnp = IntersectionNodePoint(curNode, pfInfos[curNode].prevPointId);
        insert_items(way->nodes, 0, 1, &pnp);
        curNode = edge.nextPoint.nodeId;
      }
      currentWay = (currentWay + 1) % CACHED_WAYS_NUM;
      return way;
    }
    erase_items(openNodes, shortest, 1);
    PathFinderInfo &pfInfo = pfInfos[nodeId];
    const IntersectionNode &node = nodes[nodeId];
    pfInfo.navigated = true;
    for (int i = 0; i < node.edges.size(); ++i)
    {
      const IntersectionSplineEdge &edge = node.edges[i];
      G_ASSERT(edge.nextPoint.nodeId < size);
      PathFinderInfo &nextPfInfo = pfInfos[edge.nextPoint.nodeId];
      if (nextPfInfo.navigated)
        continue;
      float distance = pfInfo.prevEdge.dist + edge.dist;
      bool proceedToNode = false;
      if (find_value_idx(openNodes, edge.nextPoint.nodeId) < 0)
      {
        openNodes.push_back(edge.nextPoint.nodeId);
        proceedToNode = true;
      }
      else
        proceedToNode = distance < nextPfInfo.prevEdge.dist;

      if (proceedToNode)
      {
        nextPfInfo.prevEdge.nextPoint.nodeId = nodeId;
        nextPfInfo.prevEdge.nextPoint.nodePointId = edge.fromPointId;
        nextPfInfo.prevEdge.dist = distance;
        nextPfInfo.prevPointId = edge.nextPoint.nodePointId;
      }
    }
  }
  return nullptr; // Nothing can be found.
}

void SplineIntersections::getNextIntersectionPoint(uint16_t fromNodeId, uint16_t toNodeId, IntersectionNodePoint &nextPoint) const
{
  G_ASSERT(fromNodeId < size);
  G_ASSERT(toNodeId < size);

  if (intersectionDataType == EIDT_TABLE)
    nextPoint = table[fromNodeId + toNodeId * size].nextPoint;
  else
  {
    int nodeNum;
    Way *way = findWay(fromNodeId, toNodeId, nodeNum);
    if (way)
      nextPoint = way->nodes[nodeNum];
    else
      nextPoint = IntersectionNodePoint(); // Invalidate, because of nowai
  }
}

float SplineIntersections::getDist(uint16_t fromNodeId, uint16_t toNodeId) const
{
  G_ASSERT(fromNodeId < size);
  G_ASSERT(toNodeId < size);

  if (intersectionDataType == EIDT_TABLE)
    return table[fromNodeId + toNodeId * size].dist;
  else
  {
    int nodeNum;
    Way *way = findWay(fromNodeId, toNodeId, nodeNum);
    float len = FLT_MAX;
    if (way)
    {
      len = way->length;
      if (nodeNum != 0)
      {
        uint16_t curPoint = way->fromNodeId;
        for (int i = 0; i < nodeNum; ++i)
        {
          const IntersectionNodePoint &pathNode = way->nodes[i];
          const IntersectionNode &beg = getIntersectionNode(curPoint);
          float minDist = FLT_MAX;
          bool found = false;
          for (int j = 0; j < beg.edges.size(); ++j)
          {
            const IntersectionSplineEdge &edge = beg.edges[j];
            if (edge.nextPoint != pathNode)
              continue;
            minDist = min(minDist, edge.dist);
            found = true;
          }
          if (found)
          {
            G_ASSERT(minDist < 1e7f);
            len -= minDist;
            if (len <= 0)
              return FLT_MAX; // same as way == null or "way not found"
          }
        }
      }
    }
    return len;
  }
}

const IntersectionNode &SplineIntersections::getIntersectionNode(uint16_t nodeId) const
{
  G_ASSERT(nodeId < nodes.size());

  return nodes[nodeId];
}

void SplineIntersections::clear()
{
  clear_and_shrink(nodes);
  clear_and_shrink(table);
  size = 0;
}

void SplineIntersections::resize(int count)
{
  G_ASSERT(count > 0 && count < 0xffff);
  size = count;
}

bool SplineIntersections::load(IGenLoad &crd)
{
  int sz = crd.readInt();

  clear();
  if (sz)
    resize(sz);

  nodes.resize(size);
  for (int i = 0; i < size; ++i)
    crd.readTab(nodes[i].points);
  int tag = crd.beginTaggedBlock();
  if (tag == _MAKE4C('tabl'))
  {
    intersectionDataType = EIDT_TABLE;
    crd.readTab(table);
  }
  else if (tag == _MAKE4C('ncon'))
  {
    intersectionDataType = EIDT_EDGES;
    for (int i = 0; i < size; ++i)
    {
      crd.read(&nodes[i].position, sizeof(Point2));
      crd.readTab(nodes[i].edges);
    }
  }
  else
    G_ASSERT(false);
  crd.endBlock();
  return true;
}

}; // namespace levelsplines
