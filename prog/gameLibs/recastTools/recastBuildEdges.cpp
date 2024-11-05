// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <recastTools/recastBuildEdges.h>
#include <recastTools/recastTools.h>

#include <generic/dag_staticTab.h>
#include <util/dag_bitArray.h>

namespace recastbuild
{
// Point between lines
// recast return heap of lines, they are connected to the contours
// this struct needs to find connected lines, it is the first step to build contours from a heap of line
struct PointOfLine
{
  IPoint3 pos;
  StaticTab<int, 4> lineArr; // idx of the lines that contain this point

  // minimally, the point belongs to one line
  PointOfLine(const IPoint3 &crd, int line)
  {
    lineArr.push_back(line);
    pos = crd;
  };

  // lineArr.erase(idx, 1);
  inline void deleteLineIdx(int idx)
  {
    lineArr[idx] = lineArr[(int)lineArr.size() - 1];
    lineArr.pop_back();
  }

  // find and delete line
  inline void deleteLine(int line)
  {
    for (int i = 0; i < (int)lineArr.size(); i++)
    {
      if (lineArr[i] == line)
      {
        deleteLineIdx(i);
        return;
      }
    }
  }

  inline void addLine(int line)
  {
    if ((int)lineArr.size() < 4)
      lineArr.push_back(line);
  }

  // get next line of the contour
  // normally in the contour, each point belongs to two lines
  // if the contours intersect, one point can belong to four lines
  // if the contour is open, the end point belongs to one line
  inline int getOtherLine(int line)
  {
    if (!lineArr.size())
      return 0;

    if (lineArr.size() == 1 && lineArr[0] == line)
      return 0;
    return lineArr[lineArr[0] == line];
  }

  // checks if these points are on the one same line or not
  inline int findCommonLineWith(const PointOfLine &other, int &idxi, int &idxj)
  {
    for (idxi = 0; idxi < (int)lineArr.size(); idxi++)
      for (idxj = 0; idxj < (int)other.lineArr.size(); idxj++)
        if (lineArr[idxi] == other.lineArr[idxj])
          return lineArr[idxi];
    return -1;
  }
};

// contour separates walkable and non walkable navmesh
// walkable navmesh can be inside or outside the contour
//
// the contour can be cut off by the border of the tile,
// in this case the contour is open: cycled = false
//
// basicly contour consists of lines, and it is easy to work
// with contour if it consist of lines (and it consistent with other recast),
// but modificate contour easier, if it consist of points.
// this structure makes it possible to work with the contour
// in the representation of points or lines, and easily switch between them
struct Contour
{
  Tab<Point3> points;
  bool cycled = false;
  BBox3 contourBox;

  Contour(IMemAlloc *mem) : points(Tab<Point3>(mem)) {}
  Contour(IMemAlloc *mem, int resv) : points(mem) { points.reserve(resv); }

  // count lines in contour
  inline int size() const { return (int)points.size() - (!cycled); }

  inline int lastIdx() const { return (int)points.size() - 1; }

  // get line
  inline Edge operator[](int line_idx) const
  {
    if (line_idx > -1)
    {
      if (cycled)
        return Edge({points[line_idx % points.size()], points[(line_idx + 1) % points.size()]});
      if (line_idx < lastIdx())
        return Edge({points[line_idx], points[line_idx + 1]});
    }
    return Edge({Point3(0.f, 0.f, 0.f), Point3(0.f, 0.f, 0.f)});
  }

  // point between two lines
  inline int commonPoint(int l1, int l2) const
  {
    if (abs(l1 - l2) == 1)
      return max(l1, l2);
    if (cycled && max(l1, l2) == lastIdx() && min(l1, l2) == 0)
      return 0;
    return -1;
  }

  // line consists of two points.
  // this function return another point of line
  inline int otherPoint(int line, int point) const
  {
    if (point < 0 || line < 0 || (point < line && line != points.size()) || point - 1 > line)
      return -1;
    if (line == points.size() && cycled)
      return point == 0 ? lastIdx() : 0;
    int res = line + (point == line);
    return res < points.size() ? res : -1;
  }

  // cycled or uncycled iterator
  inline int nextLine(int line)
  {
    if (line == -1)
      return -1;
    int nline = line + 1;
    if (nline >= points.size())
      return cycled ? 0 : -1;
    return nline;
  }

  inline void deletePoint(int pt)
  {
    if (pt >= 0 && pt < points.size())
      erase_items(points, pt, 1);
  }

  // cycled point getter
  inline Point3 getPoint(int idx) const
  {
    idx %= (int)points.size();
    if (idx < 0)
      idx += (int)points.size();
    return points[idx];
  }


  // this function return local center of contour
  // it is simple way to get direction to inside
  inline Point3 getLocalMiddle(int pt_idx)
  {
    const float smallBoxTreshold = 0.2f; // 0 - 1, 1: locBox size == contourBox size
    const float minimalBoxSize = 0.8f;   // meters
    const float minimalDist = 0.3f;      // meters
    const int minPointsCount = 5;        // minimal count points to check

    Point3 contourMiddle = contourBox.center();
    if (points.size() <= minPointsCount)
      return contourMiddle;

    float contourSizeSq = lengthSq(contourBox.width());

    int count = 1;
    Point3 startPt = getPoint(pt_idx);
    Point3 locMiddle = startPt;

    BBox3 locBox;
    locBox += startPt;

    int startIdx = pt_idx;
    int endIdx = pt_idx;

    while (count < points.size())
    {
      --startIdx;
      ++endIdx;
      count += 2;

      if (count >= points.size())
        break;

      locBox += getPoint(startIdx);
      locBox += getPoint(endIdx);
      locMiddle += getPoint(startIdx);
      locMiddle += getPoint(endIdx);
      if (count < minPointsCount)
        continue;

      float locSizeSq = lengthSq(locBox.width());
      bool isSmallBox = locSizeSq / contourSizeSq > sqr(smallBoxTreshold);

      Point3 resMiddle = isSmallBox ? locMiddle / float(count) : locBox.center();
      if (locSizeSq > sqr(minimalBoxSize) && lengthSq(startPt - resMiddle) > sqr(minimalDist))
        return resMiddle;
    }
    return contourMiddle;
  }
};

struct ContoursBuilder
{
  int m_nedges = 0;
  Tab<PointOfLine> points;
  Tab<IPoint2> lines; // line connects two points.

  float bmin[3];
  float cs;
  float ch;

  ContoursBuilder(IMemAlloc *mem, const rcContourSet *cset, const MergeEdgeParams &params_in) :
    m_nedges(MaxCountEdges(cset)), points(mem), lines(mem), params(params_in)
  {
    points.reserve(m_nedges);
    lines.reserve(m_nedges);
    maxv = (int)round((cset->bmax[0] - cset->bmin[0]) / cset->cs);
    bmin[0] = cset->bmin[0];
    bmin[1] = cset->bmin[1];
    bmin[2] = cset->bmin[2];
    cs = cset->cs;
    ch = cset->ch;

    lines.resize(m_nedges);
    points.push_back(PointOfLine(IPoint3(0, 0, 0), 0));
    lines[0] = IPoint2(0, 0);
  }

  // build points and their connections from a heap of lines
  int buildLinesConnections(const rcContourSet *m_cset)
  {
    m_nedges = 1;
    for (int i = 0; i < m_cset->nconts; ++i)
    {
      const rcContour &c = m_cset->conts[i];
      if (!c.nverts)
        continue;

      for (int j = 0; j < c.nverts; j++)
      {
        int k = (j + 1) % c.nverts;
        const IPoint3 vj = IPoint3(c.verts[j * 4], c.verts[j * 4 + 1], c.verts[j * 4 + 2]); // sq
        const IPoint3 vk = IPoint3(c.verts[k * 4], c.verts[k * 4 + 1], c.verts[k * 4 + 2]); // sp

        // clear borders
        if (isPointOnBorder(vj) && isPointOnBorder(vk) && isPointsOnOneBorder(vj, vk))
          continue;

        { // clear useless edges. it is technical edges for connect contours
          int spInd = foundEqualPoint(vk);
          int sqInd = foundEqualPoint(vj);

          if (spInd && sqInd)
          {
            int idxSp = 0, idxSq = 0;
            int commonLineIdx = points[spInd].findCommonLineWith(points[sqInd], idxSp, idxSq);
            if (commonLineIdx > -1)
            {
              points[spInd].deleteLineIdx(idxSp);
              points[sqInd].deleteLineIdx(idxSq);
              lines[commonLineIdx] = IPoint2(0, 0);
              continue;
            }
          }

          addPoint(vk, spInd, (int)m_nedges);
          addPoint(vj, sqInd, (int)m_nedges);
          lines[m_nedges++] = IPoint2(spInd, sqInd);
        }
      }
    }

    return m_nedges;
  }

  int buildContours(IMemAlloc *mem, const rcCompactHeightfield *chf, Tab<Edge> &m_edges)
  {
    m_edges.resize(m_nedges);
    m_nedges = 0;

    // cleaning points that belong to four lines
    // it happens in the intersection of two contours
    for (int ptIdx = 1; ptIdx < points.size(); ptIdx++)
    {
      if (points[ptIdx].lineArr.size() < 3)
        continue;

      int firstLine = 0;
      int secLine = 0;

      int prevLine = 0;
      int curPoint = ptIdx;
      for (int i = 0; i < points.size() * 2; i++)
      {
        int next = getNextPoint(curPoint, prevLine);
        if (i == 0)
          firstLine = prevLine;
        if (next == 0)
        {
          secLine = 0;
          break;
        }
        if (next == ptIdx)
        {
          secLine = prevLine;
          break;
        }
        curPoint = next;
      }

      if (firstLine == 0)
        continue;

      // duplicate point.
      // old point belong to the first contour, new point belong second contour
      int newPtIdx = (int)points.size();
      points.push_back(PointOfLine(points[ptIdx].pos, firstLine));
      points[ptIdx].deleteLine(firstLine);
      lines[firstLine][lines[firstLine][0] != ptIdx] = newPtIdx;

      if (secLine) // it can be open contour
      {
        points[newPtIdx].addLine(secLine);
        points[ptIdx].deleteLine(secLine);
        lines[secLine][lines[secLine][0] != ptIdx] = newPtIdx;
      }

      --ptIdx; // if contours count > 2 - repit
    }

    // check all points. each point can belong to only one contour
    Bitarray checkedPoints(mem);
    checkedPoints.resize((int)points.size());
    checkedPoints.reset();

    // build contours
    for (int ptIdx = 1; ptIdx < points.size(); ptIdx++)
    {
      if (checkedPoints[ptIdx]) // point of other contour
        continue;

      int startPoint = ptIdx;
      int curPoint = ptIdx;
      int endPoint = 0;
      int countPoints = 0;

      int prevLine = 0;  // contains previous line. it is direction of iteration
      int beginLine = 0; // it cantains start direction

      for (int i = 0; i < points.size() * 2; i++)
      {
        ++countPoints;

        int next = getNextPoint(curPoint, prevLine); // next point of contour
        if (i == 0)
          beginLine = prevLine;

        if (next == 0 || (checkedPoints[curPoint] && curPoint != startPoint)) // broken contour
        {
          checkedPoints.set(curPoint);
          if (endPoint == 0) // end of broken contour
          {
            --countPoints;
            endPoint = curPoint;
            prevLine = beginLine;  // revert check path
            curPoint = startPoint; // start point (ptIdx) != contour start (end) point. Check out the other way
            continue;
          }
          else
          {
            startPoint = curPoint; // broken contour fully checked
            break;
          }
        }

        checkedPoints.set(curPoint);

        if (next == startPoint && endPoint == 0) // end cycled contour
        {
          endPoint = startPoint;
          break;
        }
        curPoint = next;
      }

      if (countPoints < 2)
        continue;

      Contour curContour(mem, countPoints);
      curContour.cycled = startPoint == endPoint;

      prevLine = 0;
      curPoint = startPoint;

      // save discovered contour
      for (int i = 0; i < countPoints; i++)
      {
        int next = getNextPoint(curPoint, prevLine);

        Point3 pos = recastcoll::convert_point(this, points[curPoint].pos);
        curContour.points.push_back(pos);
        curContour.contourBox += pos;

        if (next == 0 || next == startPoint)
          break;
        curPoint = next;
      }

      simplificationContours(chf, curContour);

      for (int lineIdx = 0; lineIdx < curContour.size(); lineIdx++)
        m_edges[m_nedges++] = curContour[lineIdx];
    }
    m_edges.resize(m_nedges);
    return m_nedges;
  }

  void simplificationContours(const rcCompactHeightfield *chf, Contour &contour)
  {
    bool working = true;
    int curIt = 0;
    int countCircles = 10 * (int)contour.size();
    for (int i = 0; i < countCircles; i++)
    {
      if (contour.size() < 3)
        break;
      if (curIt == 0)
      {
        if (!working)
          break;
        working = false;
      }

      int midIter = contour.nextLine(curIt);
      int lastIter = contour.nextLine(midIter);

      if (tryExtrudeLines(chf, contour, curIt, midIter, lastIter))
      {
        working = true;
        continue;
      }
      else if (tryCutLines(chf, contour, curIt, midIter))
      {
        working = true;
        continue;
      }

      curIt = max(contour.nextLine(curIt), 0);
    }

    // check points height (it can be underground)
    for (Point3 &pt : contour.points)
    {
      float height;
      if (recastcoll::get_compact_heightfield_height(chf, pt, chf->cs, chf->ch, height))
        pt.y = height;
    }
  }

private:
  const MergeEdgeParams &params;
  int maxv = 0;

  int MaxCountEdges(const rcContourSet *cset)
  {
    int nedges = 1;
    for (int i = 0; i < cset->nconts; ++i)
      nedges += cset->conts[i].nverts;
    return nedges;
  }

  // check all points
  int foundEqualPoint(const IPoint3 &pt)
  {
    for (int i = (int)points.size() - 1; i > 0; i--)
      if (points[i].pos == pt)
        return (int)i;
    return 0;
  };

  // add new point, or add new line for point
  void addPoint(const IPoint3 &pt, int &ind, int line)
  {
    if (ind)
    {
      points[ind].addLine(line);
      return;
    }
    ind = (int)points.size();
    points.push_back(PointOfLine(pt, line));
  };

  bool isPointOnBorder(const IPoint3 &pt)
  {
    if (pt.x == 0 || pt.x == maxv)
      return true;
    if (pt.z == 0 || pt.z == maxv)
      return true;
    return false;
  };

  bool isPointsOnOneBorder(const IPoint3 p1, const IPoint3 p2) { return p1.x == p2.x || p1.z == p2.z; };

  // get next point in contour. prev_line is needed for direction
  int getNextPoint(int cur_pt, int &prev_line)
  {
    prev_line = points[cur_pt].getOtherLine(prev_line); // next line
    if (!prev_line)
      return 0;
    return lines[prev_line][lines[prev_line][0] == cur_pt];
  };

  // try to find common point between firstLine and secondLine, and delete midLine
  bool tryExtrudeLines(const rcCompactHeightfield *chf, Contour &contour, int firstLineIdx, int midLineIdx, int secondLineIdx)
  {
    if (firstLineIdx < 0 || midLineIdx < 0 || secondLineIdx < 0)
      return false;

    Edge firstLine = contour[firstLineIdx];
    Edge secondLine = contour[secondLineIdx];

    float mua, mub;
    lineLineIntersect(firstLine.sp, firstLine.sq, secondLine.sp, secondLine.sq, mua, mub);
    Point3 betweenSp = firstLine.sp + (firstLine.sq - firstLine.sp) * mua;
    Point3 betweenSq = secondLine.sp + (secondLine.sq - secondLine.sp) * mub;

    if (lengthSq(betweenSp - betweenSq) > params.maxExtrudeErrorSq)
      return false;

    Point3 middle = lerp(betweenSp, betweenSq, 0.5f);
    Edge midLine = contour[midLineIdx];

    float t;
    Point3 nPt;
    float distToPt = distanceSqToSeg(middle, midLine.sp, midLine.sq, t, nPt);
    if (distToPt > params.extrudeLimitSq)
      return false;

    int changeIdx = contour.commonPoint(firstLineIdx, midLineIdx);
    int delIdx = contour.commonPoint(secondLineIdx, midLineIdx);

    int fistPtIdx = contour.otherPoint(firstLineIdx, changeIdx);
    int secPtIdx = contour.otherPoint(secondLineIdx, delIdx);
    if (fistPtIdx < 0 || secPtIdx < 0 || changeIdx < 0 || delIdx < 0)
      return false;

    if (!recastcoll::is_line_walkable(chf, contour.points[fistPtIdx], middle, params.walkPrecision))
      return false;
    if (!recastcoll::is_line_walkable(chf, contour.points[secPtIdx], middle, params.walkPrecision))
      return false;

    contour.points[changeIdx] = middle;
    contour.deletePoint(delIdx);
    return true;
  }

  // try delete point between firstLine and secondLine
  bool tryCutLines(const rcCompactHeightfield *chf, Contour &contour, int firstLineIdx, int secondLineIdx)
  {
    if (firstLineIdx < 0 || secondLineIdx < 0)
      return false;

    int middlePointIdx = contour.commonPoint(firstLineIdx, secondLineIdx);
    if (middlePointIdx < 0)
      return false;

    Point3 midPoint = contour.points[middlePointIdx];

    int firstPtIdx = contour.otherPoint(firstLineIdx, middlePointIdx);
    int secPtIdx = contour.otherPoint(secondLineIdx, middlePointIdx);
    if (firstPtIdx < 0 || secPtIdx < 0)
      return false;
    Point3 firstPt = contour.points[firstPtIdx];
    Point3 secPt = contour.points[secPtIdx];

    Point3 nearestPt = closest_pt_on_line(midPoint, firstPt, secPt);
    Point3 locMiddle = contour.getLocalMiddle(middlePointIdx);
    bool unsafeCut = lengthSq(nearestPt - locMiddle) < lengthSq(midPoint - locMiddle);

    float distSq = lengthSq(Point3(nearestPt.x, 0.f, nearestPt.z) - Point3(midPoint.x, 0.f, midPoint.z));
    if (distSq > params.safeCutLimitSq)
      return false;

    if (unsafeCut)
    {
      float height;
      Point2 doublePres = params.walkPrecision * 2.f;
      if (recastcoll::get_compact_heightfield_height(chf, locMiddle, doublePres.x, 2.f, height))
      {
        locMiddle.y = height;
        unsafeCut = recastcoll::is_line_walkable(chf, nearestPt, locMiddle, doublePres);
      }
      else
        unsafeCut = false;
    }

    if (unsafeCut)
    {
      if (distSq > params.unsafeCutLimitSq)
        return false;

      float space = length(firstPt - secPt) * 0.5f * sqrt(distSq);
      if (space > params.unsafeMaxCutSpace)
        return false;
    }

    if (!recastcoll::is_line_walkable(chf, firstPt, secPt, params.walkPrecision))
      return false;

    contour.deletePoint(middlePointIdx);
    return true;
  }
};
} // namespace recastbuild

void recastbuild::build_edges(Tab<Edge> &out_edges, const rcContourSet *cset, const rcCompactHeightfield *chf,
  const MergeEdgeParams &mergeParams, RenderEdges *out_render_edges)
{
  ContoursBuilder builder(tmpmem, cset, mergeParams);

  if (builder.m_nedges == 0)
    return;

  if (builder.buildLinesConnections(cset) == 0)
    return;

  // build m_edges without simplification
  if (mergeParams.enabled)
  {
    builder.buildContours(tmpmem, chf, out_edges);
  }
  else
  {
    out_edges.reserve(builder.lines.size() - 1);
    for (int i = 1; i < builder.lines.size(); i++)
    {
      IPoint2 line = builder.lines[i];
      if (line.x < 1 || line.x >= builder.points.size() || line.y < 1 || line.y >= builder.points.size())
        continue;
      Edge newEdge;
      newEdge.sp = recastcoll::convert_point(cset, builder.points[line.x].pos);
      newEdge.sq = recastcoll::convert_point(cset, builder.points[line.y].pos);
      out_edges.push_back(newEdge);
    }
  }

  if (out_render_edges)
  {
    const int nedges = (int)out_edges.size();
    for (int i = 0; i < nedges; i++)
      out_render_edges->push_back({out_edges[i].sp, out_edges[i].sq});
  }
}
