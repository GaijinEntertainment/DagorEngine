#include <levelSplines/splineRoads.h>
#include <memory/dag_framemem.h>

namespace splineroads
{

struct PointRec
{
  uint16_t pt;
  float distSq;
  float closestPt;
  bool enabled;
  PointRec() = default;
  PointRec(uint16_t pt_, float distSq_, bool en = true) : pt(pt_), distSq(distSq_), closestPt(-1.f), enabled(en) {}
};

static void conv_spline_to_object(RoadObject &obj, const levelsplines::Spline &spl)
{
  obj.name = spl.name;

  obj.spl.calculate(&spl.pathPoints[0].bezIn, spl.pathPoints.size() * 3, false);

  int pntCount = spl.pathPoints.size();
  clear_and_resize(obj.pointData, pntCount);
  clear_and_resize(obj.pathPoints, pntCount);

  float accumulatedLen = 0.f;
  for (int j = 0; j != pntCount - 1; ++j)
  {
    obj.pointData[j] = accumulatedLen;
    accumulatedLen += obj.spl.segs[j].len;
  }
  obj.pointData[pntCount - 1] = accumulatedLen;

  obj.enabled = true;
}

bool SplineRoads::findPathPoints(int spline_id, float pos, levelsplines::IntersectionNodePoint &pt1,
  levelsplines::IntersectionNodePoint &pt2) const
{
  G_ASSERT(spline_id < roadObjects.size());
  for (int i = 0; i < (int)roadObjects[spline_id].pathPoints.size() - 1; ++i)
  {
    levelsplines::IntersectionNodePoint firstPoint = roadObjects[spline_id].pathPoints[i];
    levelsplines::IntersectionNodePoint nextPoint = roadObjects[spline_id].pathPoints[i + 1];
    G_ASSERT(firstPoint.isValid());
    G_ASSERT(nextPoint.isValid());
    const levelsplines::IntersectionNode &firstNode = intersections.getIntersectionNode(firstPoint.nodeId);
    const levelsplines::IntersectionNode &nextNode = intersections.getIntersectionNode(nextPoint.nodeId);
    if (pos >= firstNode.points[firstPoint.nodePointId].pos && pos <= nextNode.points[nextPoint.nodePointId].pos)
    {
      pt1 = firstPoint;
      pt2 = nextPoint;
      return true;
    }
  }
  return false;
}

bool SplineRoads::findWay(const Point3 &src, const Point3 &dst, float dist_around, int &spline_id, float &src_spl_point_t,
  float &dst_spl_point_t, uint16_t &node_id, uint16_t &dest_node_id) const
{
  int srcSplId = -1, dstSplId = -1;
  float srcSplPt = 0.f, dstSplPt = 0.f;
  if (!getRoadAround(src, dst, dist_around, srcSplId, dstSplId, srcSplPt, dstSplPt, false))
    return false;

  levelsplines::IntersectionNodePoint srcPoint[2];
  levelsplines::IntersectionNodePoint dstPoint[2];
  if (!findPathPoints(srcSplId, srcSplPt, srcPoint[0], srcPoint[1]) || !findPathPoints(dstSplId, dstSplPt, dstPoint[0], dstPoint[1]))
    return false;

  float minDist = FLT_MAX;
  for (int i = 0; i < countof(srcPoint); ++i)
  {
    for (int j = 0; j < countof(dstPoint); ++j)
    {
      const levelsplines::IntersectionNode &intersecNode = intersections.getIntersectionNode(srcPoint[i].nodeId);
      const levelsplines::IntersectionNode &dstintersecNode = intersections.getIntersectionNode(dstPoint[j].nodeId);
      const levelsplines::IntersectionSplinePoint &splinePoint = intersecNode.points[srcPoint[i].nodePointId];
      const levelsplines::IntersectionSplinePoint &dstSplinePoint = dstintersecNode.points[dstPoint[j].nodePointId];
      float dist = intersections.getDist(srcPoint[i].nodeId, dstPoint[j].nodeId) + rabs(srcSplPt - splinePoint.pos) +
                   rabs(dstSplPt - dstSplinePoint.pos);
      if (dist < minDist)
      {
        G_ASSERT(splinePoint.splineId == srcSplId);

        spline_id = srcSplId;
        src_spl_point_t = srcSplPt;
        dst_spl_point_t = splinePoint.pos;
        node_id = srcPoint[i].nodeId;
        dest_node_id = dstPoint[j].nodeId;

        minDist = dist;
      }
    }
  }

  if (minDist > 1e7f)
    return false;

  return true;
}

void SplineRoads::nextPointToDest(uint16_t &node_id, uint16_t dst, int &spline_id, float &src_spl_point_t,
  float &dst_spl_point_t) const
{
  G_ASSERT(node_id < 0xffff);
  G_ASSERT(dst < 0xffff);

  uint16_t srcNodeId = node_id;
  levelsplines::IntersectionNodePoint nextPoint;
  intersections.getNextIntersectionPoint(srcNodeId, dst, nextPoint);
  node_id = nextPoint.nodeId;
  uint8_t nodePointId = nextPoint.nodePointId;
  if (nextPoint.isValid())
  {
    const levelsplines::IntersectionNode &srcNode = intersections.getIntersectionNode(srcNodeId);
    const levelsplines::IntersectionNode &dstNode = intersections.getIntersectionNode(node_id);
    spline_id = int(dstNode.points[nodePointId].splineId);
    dst_spl_point_t = dstNode.points[nodePointId].pos;
    float minDist = FLT_MAX;
    for (int i = 0; i < srcNode.points.size(); ++i)
    {
      if (srcNode.points[i].splineId == spline_id)
      {
        float dist = rabs(srcNode.points[i].pos - dst_spl_point_t);
        if (dist < minDist)
        {
          minDist = dist;
          src_spl_point_t = srcNode.points[i].pos;
          src_spl_point_t = dst_spl_point_t > src_spl_point_t ? min(src_spl_point_t + 10.f, dst_spl_point_t)
                                                              : max(src_spl_point_t - 10.f, dst_spl_point_t);
        }
      }
    }
    G_ASSERT(minDist < 1e7f);
  }
}

bool SplineRoads::getRoadAround(const Point3 &src, const Point3 &dst, float dist_around, int &src_spline_id, int &dst_spline_id,
  float &pt_src, float &pt_dst, bool same_spline, bool dont_turn_around) const
{
  if (roadsGrid.grid.empty() || roadsGrid.splinePoints.empty())
    return false;

  dont_turn_around &= same_spline;
  IPoint2 minSrc = get_spline_grid_cell(roadsGrid, src - Point3(dist_around, 0.f, dist_around));
  IPoint2 maxScr = get_spline_grid_cell(roadsGrid, src + Point3(dist_around, 0.f, dist_around));
  IPoint2 minDst = get_spline_grid_cell(roadsGrid, dst - Point3(dist_around, 0.f, dist_around));
  IPoint2 maxDst = get_spline_grid_cell(roadsGrid, dst + Point3(dist_around, 0.f, dist_around));
  Point2 src2D = Point2::xz(src);
  Point2 dst2D = Point2::xz(dst);

  // Src search.
  Tab<PointRec> srcSplinePoints(framemem_ptr());
  srcSplinePoints.resize(roadObjects.size());
  Tab<PointRec> dstSplinePoints(framemem_ptr());
  dstSplinePoints.resize(roadObjects.size());

  // init.
  for (int i = 0; i != roadObjects.size(); ++i)
  {
    PointRec recSrc(0, (src2D - getRoadPoint2D(i, 0)).lengthSq(), roadObjects[i].enabled);
    srcSplinePoints[i] = recSrc;
    PointRec recDst(0, (dst2D - getRoadPoint2D(i, 0)).lengthSq(), roadObjects[i].enabled);
    dstSplinePoints[i] = recDst;
  }

  // Cycle through all points around src.
  for (int i = minSrc.x; i != maxScr.x + 1; ++i)
  {
    for (int j = minSrc.y; j != maxScr.y + 1; ++j)
    {
      int clampedTop = roadsGrid.grid[j * roadsGrid.width + i + 1];
      for (int num = roadsGrid.grid[j * roadsGrid.width + i]; num != clampedTop && num < roadsGrid.splinePoints.size(); ++num)
      {
        levelsplines::GridPoint pt = roadsGrid.splinePoints[num];
        if (!srcSplinePoints[pt.splineId].enabled)
          continue;
        float distSq = (src2D - getRoadPoint2D(pt.splineId, pt.pointId)).lengthSq();
        if (distSq < srcSplinePoints[pt.splineId].distSq)
          srcSplinePoints[pt.splineId] = PointRec(pt.pointId, distSq);
      }
    }
  }

  // Early check.
  bool noSrcPoints = true;
  for (int i = 0; i != roadObjects.size(); ++i)
  {
    srcSplinePoints[i].closestPt = getClosest(Point2::xz(src), i, srcSplinePoints[i].pt, srcSplinePoints[i].distSq);
    if (srcSplinePoints[i].distSq < sqr(dist_around))
      noSrcPoints = false;
  }

  if (noSrcPoints)
    return false;

  // Dst search
  for (int i = minDst.x; i != maxDst.x + 1; ++i)
  {
    for (int j = minDst.y; j != maxDst.y + 1; ++j)
    {
      int clampedTop = roadsGrid.grid[j * roadsGrid.width + i + 1];
      for (int num = roadsGrid.grid[j * roadsGrid.width + i]; num != clampedTop; ++num)
      {
        levelsplines::GridPoint pt = roadsGrid.splinePoints[num];
        if (!dstSplinePoints[pt.splineId].enabled)
          continue;
        float distSq = (dst2D - getRoadPoint2D(pt.splineId, pt.pointId)).lengthSq();
        if (distSq < dstSplinePoints[pt.splineId].distSq)
          dstSplinePoints[pt.splineId] = PointRec(pt.pointId, distSq);
      }
    }
  }

  bool noDstPoints = true;
  // int minSplineId = -1;
  int minSrcSplineId = -1;
  int minDstSplineId = -1;
  float minDistSq = FLT_MAX;
  float minSrcDist = FLT_MAX;
  float minDstDist = FLT_MAX;
  for (int i = 0; i < roadObjects.size(); ++i)
  {
    dstSplinePoints[i].closestPt = getClosest(Point2::xz(dst), i, dstSplinePoints[i].pt, dstSplinePoints[i].distSq);
    if (dstSplinePoints[i].distSq < sqr(dist_around) && srcSplinePoints[i].distSq < sqr(dist_around))
    {
      noDstPoints = false;
      if (dont_turn_around && src_spline_id == i &&
          ((srcSplinePoints[i].closestPt - dstSplinePoints[i].closestPt) * (pt_src - pt_dst)) < 0)
        continue;
    }
    else if (same_spline)
      continue;
    if (same_spline)
    {
      float distSq = srcSplinePoints[i].distSq + dstSplinePoints[i].distSq;
      if (distSq < minDistSq)
      {
        minSrcSplineId = minDstSplineId = i;
        minDistSq = distSq;
      }
    }
    else
    {
      if (srcSplinePoints[i].distSq < minSrcDist)
      {
        minSrcSplineId = i;
        minSrcDist = srcSplinePoints[i].distSq;
      }
      if (dstSplinePoints[i].distSq < minDstDist)
      {
        minDstSplineId = i;
        minDstDist = dstSplinePoints[i].distSq;
      }
    }
  }

  if (same_spline ? noDstPoints : (minSrcDist > sqr(dist_around) || minDstDist > sqr(dist_around)))
    return false;

  // Finally, we can say, that everything is set, so we can fill all values and return true.
  if (dont_turn_around && minSrcSplineId < 0)
    minSrcSplineId = minDstSplineId = src_spline_id;
  src_spline_id = minSrcSplineId;
  dst_spline_id = minDstSplineId;
  pt_src = srcSplinePoints[src_spline_id].closestPt;
  pt_dst = dstSplinePoints[dst_spline_id].closestPt;
  if (same_spline)
  {
    if (pt_dst > pt_src)
      pt_src = min(pt_src + 10.f, pt_dst);
    else
      pt_src = max(pt_src - 10.f, pt_dst);
  }
  return true;
}

Point3 SplineRoads::getRoadPoint(int spline_id, float pt) const { return roadObjects[spline_id].spl.get_pt(pt); }

Point3 SplineRoads::getRoadPoint(int spline_id, int point_id) const
{
  G_ASSERT(spline_id >= 0);
  G_ASSERT(spline_id < roadObjects.size());
  G_ASSERT(point_id >= 0);
  G_ASSERTF(point_id < roadObjects[spline_id].pointData.size(), "pointId = %d", point_id);
  return getRoadPoint(spline_id, roadObjects[spline_id].pointData[point_id]);
}

Point2 SplineRoads::getRoadPoint2D(int spline_id, float pt) const { return Point2::xz(getRoadPoint(spline_id, pt)); }

Point2 SplineRoads::getRoadPoint2D(int spline_id, int point_id) const { return Point2::xz(getRoadPoint(spline_id, point_id)); }

TMatrix SplineRoads::getRoadPointMat(int spline_id, float pt) const
{
  TMatrix result;
  Point3 forward = roadObjects[spline_id].spl.get_tang(pt);
  forward = normalize(Point3::x0z(forward));
  Point3 up = Point3(0.f, 1.f, 0.f);
  Point3 right = forward % up;
  result.setcol(0, right);
  result.setcol(1, up);
  result.setcol(2, forward);
  result.setcol(3, getRoadPoint(spline_id, pt));
  return result;
}

TMatrix SplineRoads::getRoadPointMat(int spline_id, int point_id) const
{
  return getRoadPointMat(spline_id, roadObjects[spline_id].pointData[point_id]);
}

float SplineRoads::getRoadPointT(int spline_id, int point_id) const
{
  G_ASSERT(spline_id >= 0);
  G_ASSERT(spline_id < roadObjects.size());
  G_ASSERT(point_id >= 0);
  G_ASSERT(point_id < roadObjects[spline_id].pointData.size());
  return roadObjects[spline_id].pointData[point_id];
}

float SplineRoads::getClosest(const Point2 &pt, int spline_id, int around, float &dist) const
{
  uint16_t splTop = roadObjects[spline_id].pointData.size();
  uint16_t prevPt = max(around - 1, 0);
  uint16_t nextPt = min(around + 1, int(splTop) - 1);
  Point2 c = getRoadPoint2D(spline_id, around);
  Point2 cprev = getRoadPoint2D(spline_id, prevPt);
  Point2 cnext = getRoadPoint2D(spline_id, nextPt);

  Point2 topt = pt - c;
  Point2 ctop = cprev - c;
  Point2 cton = cnext - c;
  float dprev = clamp(topt * ctop / (ctop.lengthSq() + 1e-9f), 0.f, 1.f);
  float dnext = clamp(topt * cton / (cton.lengthSq() + 1e-9f), 0.f, 1.f);
  Point2 prevPoint = c + ctop * dprev;
  Point2 nextPoint = c + cton * dnext;
  float dpDist = (pt - prevPoint).lengthSq();
  float dnDist = (pt - nextPoint).lengthSq();
  dist = min(dpDist, dnDist);
  if (dpDist < dnDist)
    return lerp(getRoadPointT(spline_id, around), getRoadPointT(spline_id, prevPt), dprev);
  else
    return lerp(getRoadPointT(spline_id, around), getRoadPointT(spline_id, nextPt), dnext);
}

bool SplineRoads::loadRoadObjects(dag::ConstSpan<levelsplines::Spline> splines)
{
  intersections.clear();
  for (const levelsplines::Spline &spl : splines)
  {
    RoadObject &conv = roadObjects.push_back();
    conv_spline_to_object(conv, spl);
  }
  return true;
}

bool SplineRoads::loadRoadObjects(IGenLoad &crd)
{
  Tab<levelsplines::Spline> splines(framemem_ptr());
  levelsplines::load_splines(crd, splines);
  return loadRoadObjects(splines);
}

bool SplineRoads::loadRoadGrid(IGenLoad &crd)
{
  load_grid(crd, roadsGrid);
  return true;
}

bool SplineRoads::loadIntersections(IGenLoad &crd)
{
  if (!intersections.load(crd))
    return false;

  for (int i = 0; i < roadObjects.size(); ++i)
    crd.readTab(roadObjects[i].pathPoints);
  return true;
}

}; // namespace splineroads
